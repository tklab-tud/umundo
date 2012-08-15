/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#include "umundo/connection/zeromq/ZeroMQNode.h"

#include "umundo/connection/Publisher.h"
#include "umundo/common/Message.h"
#include "umundo/common/UUID.h"

// include order matters with MSVC ...
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

#include "umundo/config.h"
#if defined UNIX || defined IOS || defined IOSSIM
#include <stdio.h> // snprintf
#endif

namespace umundo {

shared_ptr<Implementation> ZeroMQSubscriber::create(void*) {
	shared_ptr<Implementation> instance(new ZeroMQSubscriber());
	return instance;
}

void ZeroMQSubscriber::destroy() {
	delete(this);
}

ZeroMQSubscriber::ZeroMQSubscriber() {
}

ZeroMQSubscriber::~ZeroMQSubscriber() {
	LOG_INFO("deleting subscriber for %s", _channelName.c_str());
	stop();
	join();
	zmq_close(_socket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
}

void ZeroMQSubscriber::init(shared_ptr<Configuration> config) {
	_config = boost::static_pointer_cast<SubscriberConfig>(config);
	_uuid = (_uuid.length() > 0 ? _uuid : UUID::getUUID());
	assert(_uuid.length() == 36);

	void* ctx = ZeroMQNode::getZeroMQContext();
	(_socket = zmq_socket(ctx, ZMQ_SUB)) || LOG_WARN("zmq_socket: %s",zmq_strerror(errno));

	assert(_channelName.size() > 0);
	int hwm = NET_ZEROMQ_RCV_HWM;
	zmq_setsockopt(_socket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_socket, ZMQ_IDENTITY, _uuid.c_str(), _uuid.length()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_socket, ZMQ_SUBSCRIBE, _channelName.c_str(), _channelName.size()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	LOG_DEBUG("Subscribing to %s", _channelName.c_str());
	zmq_setsockopt(_socket, ZMQ_SUBSCRIBE, _uuid.c_str(), _uuid.size()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	LOG_DEBUG("Subscribing to %s", SHORT_UUID(_uuid).c_str());

	// reconnection intervals
	int reconnect_ivl_min = 100;
	int reconnect_ivl_max = 200;
	zmq_setsockopt (_socket, ZMQ_RECONNECT_IVL, &reconnect_ivl_min, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt (_socket, ZMQ_RECONNECT_IVL_MAX, &reconnect_ivl_max, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));

	LOG_INFO("creating subscriber for %s", _channelName.c_str());
	start();
}

void ZeroMQSubscriber::suspend() {
	UMUNDO_LOCK(_mutex);
	if (_isSuspended) {
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	_isSuspended = true;
	stop();
	join();

	_connections.clear();
	zmq_close(_socket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));

	UMUNDO_UNLOCK(_mutex);
}

void ZeroMQSubscriber::resume() {
	UMUNDO_LOCK(_mutex);
	if (!_isSuspended) {
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	_isSuspended = false;
	init(_config);

	UMUNDO_UNLOCK(_mutex);
}

void ZeroMQSubscriber::run() {
	int32_t more;
	size_t more_size = sizeof(more);

	zmq_pollitem_t pollItem;
	pollItem.socket = _socket;
	pollItem.events = ZMQ_POLLIN | ZMQ_POLLOUT;

	while(isStarted()) {
		int rc = 0;
		{
			ScopeLock lock(&_mutex);
			rc = zmq_poll(&pollItem, 1, 30);
		}
		if (rc < 0) {
			// an error occurred
			switch(rc) {
			case ETERM:
				LOG_WARN("zmq_poll called with terminated socket");
				break;
			case EFAULT:
				LOG_WARN("zmq_poll called with invalid items");
				break;
			case EINTR:
			default:
				break;
			}

		} else if (rc == 0) {
			// Nothing to read - released lock
			Thread::yield();
//			Thread::sleepMs(5);

		}
		if (rc > 0) {
			// there is a message to be read
			ScopeLock lock(&_mutex);
			Message* msg = new Message();
			while (1) {
				// read and dispatch the whole message
				zmq_msg_t message;
				zmq_msg_init(&message) && LOG_WARN("zmq_msg_init: %s",zmq_strerror(errno));

				while (zmq_recvmsg(_socket, &message, 0) < 0)
					if (errno != EINTR)
						LOG_WARN("zmq_recvmsg: %s",zmq_strerror(errno));

				size_t msgSize = zmq_msg_size(&message);

				if (!isStarted()) {
					zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
					return;
				}

				// last message contains actual data
				zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size) && LOG_WARN("zmq_getsockopt: %s",zmq_strerror(errno));

				if (more) {
					char* key = (char*)zmq_msg_data(&message);
					char* value = ((char*)zmq_msg_data(&message) + strlen(key) + 1);

					// is this the first message with the channelname?
					if (strlen(key) + 1 == msgSize &&
					        msg->getMeta().find(key) == msg->getMeta().end()) {
						msg->putMeta("channel", key);
					} else {
						assert(strlen(key) + strlen(value) + 2 == msgSize);
						if (strlen(key) + strlen(value) + 2 != msgSize) {
							LOG_WARN("Received malformed message %d + %d + 2 != %d", strlen(key), strlen(value), msgSize);
							zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
							break;
						} else {
							msg->putMeta(key, value);
						}
					}
					zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
				} else {
					msg->setData((char*)zmq_msg_data(&message), msgSize);
					zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

					if (_receiver != NULL) {
						_receiver->receive(msg);
						delete(msg);
					} else {
						ScopeLock lock(&_msgMutex);
						_msgQueue.push_back(msg);
					}
					break; // last message part
				}
			}
		}
	}
}

Message* ZeroMQSubscriber::getNextMsg() {
	if (_receiver != NULL) {
		LOG_WARN("getNextMsg not functional when a receiver is given");
		return NULL;
	}

	ScopeLock lock(&_msgMutex);
	Message* msg = NULL;
	if (_msgQueue.size() > 0) {
		msg = _msgQueue.front();
		_msgQueue.pop_front();
	}
	return msg;
}

Message* ZeroMQSubscriber::peekNextMsg() {
	if (_receiver != NULL) {
		LOG_WARN("peekNextMsg not functional when a receiver is given");
		return NULL;
	}

	ScopeLock lock(&_msgMutex);
	Message* msg = NULL;
	if (_msgQueue.size() > 0) {
		msg = _msgQueue.front();
	}
	return msg;
}

void ZeroMQSubscriber::added(shared_ptr<PublisherStub> pub) {
	std::stringstream ss;
	if (pub->isInProcess() && false) {
		ss << "inproc://" << pub->getUUID();
	} else {
		ss << pub->getTransport() << "://" << pub->getIP() << ":" << pub->getPort();
	}
	if (_connections.find(ss.str()) != _connections.end()) {
		LOG_INFO("Already connected to %s", ss.str().c_str());
		return;
	}

	ScopeLock lock(&_mutex);
	_pubUUIDs.insert(pub->getUUID());
	LOG_INFO("%s subscribing at %s", _channelName.c_str(), ss.str().c_str());
	zmq_connect(_socket, ss.str().c_str()) && LOG_WARN("zmq_connect: %s", zmq_strerror(errno));
	_connections.insert(ss.str());
}

void ZeroMQSubscriber::removed(shared_ptr<PublisherStub> pub) {
	ScopeLock lock(&_mutex);
	std::stringstream ss;
	if (pub->isInProcess() && false) {
		ss << "inproc://" << pub->getUUID();
	} else {
		ss << pub->getTransport() << "://" << pub->getIP() << ":" << pub->getPort();
	}
	if (_connections.find(ss.str()) == _connections.end()) {
		LOG_INFO("Never connected to %s won't disconnect", ss.str().c_str());
		return;
	}

	_pubUUIDs.erase(pub->getUUID());

	LOG_DEBUG("unsubscribing from %s", ss.str().c_str());
	zmq_connect(_socket, ss.str().c_str()) && LOG_WARN("zmq_disconnect: %s", zmq_strerror(errno));
	_connections.erase(ss.str());
}

void ZeroMQSubscriber::changed(shared_ptr<PublisherStub>) {
	// never called
}


}