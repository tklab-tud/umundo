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

ZeroMQSubscriber::ZeroMQSubscriber() {}

void ZeroMQSubscriber::init(boost::shared_ptr<Configuration> config) {
	_config = boost::static_pointer_cast<SubscriberConfig>(config);

	(_subSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_SUB))     || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_readOpSocket  = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_writeOpSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_bind(_readOpSocket, readOpId.c_str())  && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
	zmq_connect(_writeOpSocket, readOpId.c_str()) && LOG_ERR("zmq_connect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	assert(_channelName.size() > 0);

	int hwm = NET_ZEROMQ_RCV_HWM;
	std::string subId("um.sub." + _uuid);
	std::string lastSub("~" + _uuid); ///< this needs to have very "late" alphabetical order to ensure all channels are subscribed to first

	zmq_setsockopt(_subSocket, ZMQ_IDENTITY, subId.c_str(), subId.length()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, _channelName.c_str(), _channelName.length())  && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, lastSub.c_str(), lastSub.length())  && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));

	int rcvTimeOut = 30;
	zmq_setsockopt(_subSocket, ZMQ_RCVTIMEO, &rcvTimeOut, sizeof(rcvTimeOut));

	// reconnection intervals
#if 0
	int reconnect_ivl_min = 100;
	int reconnect_ivl_max = 200;
	zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL, &reconnect_ivl_min, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL_MAX, &reconnect_ivl_max, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
#endif

	zmq_bind(_subSocket, std::string("inproc://" + subId).c_str());
}

ZeroMQSubscriber::~ZeroMQSubscriber() {
	LOG_INFO("deleting subscriber for %s", _channelName.c_str());
	stop();
	join();
	zmq_close(_subSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_readOpSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_writeOpSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
}

boost::shared_ptr<Implementation> ZeroMQSubscriber::create() {
	return boost::shared_ptr<ZeroMQSubscriber>(new ZeroMQSubscriber());
}

void ZeroMQSubscriber::suspend() {
	ScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;

};

void ZeroMQSubscriber::resume() {
	ScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;
};

void ZeroMQSubscriber::added(PublisherStub pub) {
	ScopeLock lock(_mutex);
	if (_pubUUIDs.find(pub.getUUID()) != _pubUUIDs.end())
		return;

	_pubUUIDs.insert(pub.getUUID());

	if (_domainPubs.count(pub.getDomain()) == 0) {
		std::stringstream ss;

		if (pub.isInProcess()) {
			// same process, use inproc communication
			ss << "inproc://um.pub." << pub.getDomain();
		} else if (!pub.isRemote() && false) { // disabled for now
			// same host, use inter-process communication
			ss << "ipc://um.pub." << pub.getDomain();
		} else {
			// remote node, use network
			ss << pub.getTransport() << "://" << pub.getIP() << ":" << pub.getPort();
		}

		if (isStarted()) {
			ZMQ_INTERNAL_SEND("connectPub", ss.str().c_str());
		} else {
			zmq_connect(_subSocket, ss.str().c_str()) && LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));
		}
	}

	_pubUUIDs.insert(pub.getUUID());
	_domainPubs.insert(std::make_pair(pub.getDomain(), pub.getUUID()));
}

void ZeroMQSubscriber::removed(PublisherStub pub) {
	ScopeLock lock(_mutex);
	if (_pubUUIDs.find(pub.getUUID()) == _pubUUIDs.end())
		return;

	assert(_domainPubs.count(pub.getDomain()) != 0);
	std::multimap<std::string, std::string>::iterator domIter = _domainPubs.find(pub.getDomain());
	while(domIter != _domainPubs.end()) {
		if (domIter->second == pub.getUUID()) {
			_domainPubs.erase(domIter++);
		} else {
			domIter++;
		}
	}
	_pubUUIDs.erase(pub.getUUID());

	if (_domainPubs.count(pub.getDomain()) == 0) {
		std::stringstream ss;
		if (pub.isInProcess()) {
			// same process, use inproc communication
			ss << "inproc://um.pub." << pub.getDomain();
		} else if (!pub.isRemote() && false) { // disabled for now
			// same host, use inter-process communication
			ss << "ipc://um.pub." << pub.getDomain();
		} else {
			// remote node, use network
			ss << pub.getTransport() << "://" << pub.getIP() << ":" << pub.getPort();
		}

		if (isStarted()) {
			ZMQ_INTERNAL_SEND("disconnectPub", ss.str().c_str());
		} else {
			zmq_disconnect(_subSocket, ss.str().c_str()) && LOG_ERR("zmq_disconnect %s: %s", ss.str().c_str(), zmq_strerror(errno));
		}
	}
}

void ZeroMQSubscriber::changed(PublisherStub pub) {
}

void ZeroMQSubscriber::setReceiver(Receiver* receiver) {
	stop();
	join();
	_receiver = receiver;
	if (_receiver != NULL) {
		start();
	}
}

void ZeroMQSubscriber::run() {
	zmq_pollitem_t items [] = {
		{ _readOpSocket, 0, ZMQ_POLLIN, 0 }, // one of our members wants to manipulate a socket
		{ _subSocket,    0, ZMQ_POLLIN, 0 }, // publication requests
	};

	while(isStarted()) {
		int rc = zmq_poll(items, 2, 20);
		if (rc < 0) {
			LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
		}

		if (!isStarted())
			return;

		if (items[0].revents & ZMQ_POLLIN) {
			/**
			 * Node internal request from member methods for socket operations
			 * We need this here for thread safety.
			 */
			while (1) {
				int more;
				size_t more_size = sizeof (more);

				zmq_msg_t message;
				zmq_msg_t endpointMsg;

				zmq_msg_init (&message);
				zmq_msg_recv (&message, _readOpSocket, 0);
				char* op = (char*)zmq_msg_data(&message);

				zmq_msg_init (&endpointMsg);
				zmq_msg_recv (&endpointMsg, _readOpSocket, 0);
				char* endpoint = (char*)zmq_msg_data(&endpointMsg);

				if (false) {
				} else if (strcmp(op, "connectPub") == 0) {
					zmq_connect(_subSocket, endpoint) && LOG_ERR("zmq_connect %s: %s", endpoint, zmq_strerror(errno));
				} else if (strcmp(op, "disconnectPub") == 0) {
					zmq_disconnect(_subSocket, endpoint) && LOG_ERR("zmq_disconnect %s: %s", endpoint, zmq_strerror(errno));
				}

				zmq_getsockopt (_readOpSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&message);
				zmq_msg_close (&endpointMsg);

				assert(!more); // we read all messages
				if (!more)
					break;      //  Last message part
			}
		}

		if (items[1].revents & ZMQ_POLLIN && _receiver != NULL) {
			Message* msg = getNextMsg();
			_receiver->receive(msg);
		}
	}
}

Message* ZeroMQSubscriber::getNextMsg() {
	int32_t more;
	size_t more_size = sizeof(more);

	Message* msg = new Message();
	while (1) {
		// read the whole message
		zmq_msg_t message;
		zmq_msg_init(&message) && LOG_WARN("zmq_msg_init: %s",zmq_strerror(errno));

		int rc;
		rc = zmq_recvmsg(_subSocket, &message, ZMQ_DONTWAIT);
		if (rc < 0) {
			LOG_WARN("zmq_recvmsg: %s",zmq_strerror(errno));
			zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
			delete msg;
			return NULL;
		}

		size_t msgSize = zmq_msg_size(&message);
		zmq_getsockopt(_subSocket, ZMQ_RCVMORE, &more, &more_size) && LOG_WARN("zmq_getsockopt: %s",zmq_strerror(errno));

		if (more) {
			char* key = (char*)zmq_msg_data(&message);
			char* value = ((char*)zmq_msg_data(&message) + strlen(key) + 1);

			// is this the first message with the channelname?
			if (strlen(key) + 1 == msgSize &&
			        msg->getMeta().find(key) == msg->getMeta().end()) {
				msg->putMeta("um.channel", key);
			} else {
				if (strlen(key) + strlen(value) + 2 != msgSize) {
					LOG_ERR("Received malformed meta field %d + %d + 2 != %d", strlen(key), strlen(value), msgSize);
					zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
					break;
				} else {
					msg->putMeta(key, value);
				}
			}
			zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
		} else {
			// last message contains actual data
			msg->setData((char*)zmq_msg_data(&message), msgSize);
			zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
			break; // last message part
		}
	}
	return msg;
}

bool ZeroMQSubscriber::hasNextMsg() {
	zmq_pollitem_t items[1];
	items[0].socket = _subSocket;
	items[0].events = ZMQ_POLLIN;

	int rc = zmq_poll(items, 1, 0);
	if (rc < 0) {
		LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
		return false;
	}
	if (items[0].revents & ZMQ_POLLIN) {
		return true;
	}
	return false;
}


}