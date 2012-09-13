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

#ifdef WIN32
#include <time.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#endif

#include "umundo/connection/zeromq/ZeroMQPublisher.h"

#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/common/Message.h"
#include "umundo/common/UUID.h"
#include "umundo/common/Host.h"

#include "umundo/config.h"
#if defined UNIX || defined IOS || defined IOSSIM
#include <string.h> // strlen, memcpy
#include <stdio.h> // snprintf
#endif

namespace umundo {

shared_ptr<Implementation> ZeroMQPublisher::create(void* facade) {
	shared_ptr<Implementation> instance(new ZeroMQPublisher());
	boost::static_pointer_cast<ZeroMQPublisher>(instance)->_facade = facade;
	return instance;
}

void ZeroMQPublisher::destroy() {
	delete(this);
}

void ZeroMQPublisher::init(shared_ptr<Configuration> config) {
	ScopeLock lock(_mutex);
	_uuid = (_uuid.length() > 0 ? _uuid : UUID::getUUID());
	_config = boost::static_pointer_cast<PublisherConfig>(config);
	_transport = "tcp";

	(_socket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_XPUB)) || LOG_WARN("zmq_socket: %s",zmq_strerror(errno));

	int hwm = NET_ZEROMQ_SND_HWM;
	zmq_setsockopt(_socket, ZMQ_SNDHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));

	std::stringstream ssInProc;
	ssInProc << "inproc://" << _uuid;
	zmq_bind(_socket, ssInProc.str().c_str()) && LOG_WARN("zmq_bind: %s",zmq_strerror(errno));
	LOG_INFO("creating publisher for %s on uuid %s", _channelName.c_str(), ssInProc.str().c_str());

#ifndef PUBPORT_SHARING
	_port = ZeroMQNode::bindToFreePort(_socket, _transport, "*");
	LOG_INFO("creating publisher for %s on port %d", _channelName.c_str(), _port);
	start();
#else

	_port = ZeroMQNode::_sharedPubPort;
#endif

	LOG_DEBUG("creating publisher on %s", ssInProc.str().c_str());
}

ZeroMQPublisher::ZeroMQPublisher() {
}

ZeroMQPublisher::~ZeroMQPublisher() {
	LOG_INFO("deleting publisher for %s", _channelName.c_str());
  
	stop();
	join();
	zmq_close(_socket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));

  // clean up pending messages
  map<string, std::list<std::pair<uint64_t, Message*> > >::iterator queuedMsgHostIter = _queuedMessages.begin();
  while(queuedMsgHostIter != _queuedMessages.end()) {
    std::list<std::pair<uint64_t, Message*> >::iterator queuedMsgIter = queuedMsgHostIter->second.begin();
    while(queuedMsgIter != queuedMsgHostIter->second.end()) {
      delete (queuedMsgIter->second);
      queuedMsgIter++;
    }
    queuedMsgHostIter++;
  }

}

void ZeroMQPublisher::join() {
	Thread::join();
}

void ZeroMQPublisher::suspend() {
	if (_isSuspended)
		return;
	ScopeLock lock(_mutex);

	_isSuspended = true;

	stop();
	join();

	zmq_close(_socket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
}

void ZeroMQPublisher::resume() {
	if (!_isSuspended)
		return;
	_isSuspended = false;
	init(_config);
}

void ZeroMQPublisher::run() {
	// read subscription requests from the pub socket
	while(isStarted()) {
		zmq_msg_t message;
		zmq_msg_init(&message) && LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno));
		int rv;
		{
			ScopeLock lock(_mutex);
			while ((rv = zmq_recvmsg(_socket, &message, ZMQ_DONTWAIT)) < 0) {
				if (errno == EAGAIN) // no messages available at the moment
					break;
				if (errno != EINTR)
					LOG_WARN("zmq_recvmsg: %s",zmq_strerror(errno));
			}
		}

		if (rv > 0) {
			size_t msgSize = zmq_msg_size(&message);
			// every subscriber will sent its uuid as a subscription as well
			if (msgSize == 37) {
				//ScopeLock lock(&_mutex);

				char* data = (char*)zmq_msg_data(&message);
				bool subscription = (data[0] == 0x1);
				char* subId = data+1;
				subId[msgSize - 1] = 0;

				if (subscription) {
					LOG_INFO("%s received ZMQ subscription from %s", _channelName.c_str(), subId);
					_pendingZMQSubscriptions.insert(subId);
					addedSubscriber("", subId);
				} else {
					LOG_INFO("%s received ZMQ unsubscription from %s", _channelName.c_str(), subId);
				}
			}
			zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s", zmq_strerror(errno));
		} else {
			Thread::sleepMs(50);
		}
    
    // try to send pending messages
    ScopeLock lock(_mutex);
    if (_queuedMessages.size() > 0) {
      uint64_t now = Thread::getTimeStampMs();
      map<string, std::list<std::pair<uint64_t, Message*> > >::iterator queuedMsgHostIter = _queuedMessages.begin();
      while(queuedMsgHostIter != _queuedMessages.end()) {
        std::list<std::pair<uint64_t, Message*> >::iterator queuedMsgIter = queuedMsgHostIter->second.begin();
        if (_subUUIDs.find(queuedMsgHostIter->first) != _subUUIDs.end()) {
          // node became available, send pending messages
          LOG_INFO("Subscriber %s became available on %s - sending %d pending message", queuedMsgHostIter->first.c_str(), _channelName.c_str(), queuedMsgHostIter->second.size());
          while(queuedMsgIter != queuedMsgHostIter->second.end()) {
            send(queuedMsgIter->second);
            delete queuedMsgIter->second;
            queuedMsgHostIter->second.erase(queuedMsgIter++);
          }
        } else {
          while(queuedMsgIter != queuedMsgHostIter->second.end()) {
            if (now - queuedMsgIter->first > 5000) {
              LOG_ERR("Pending message for %s on %s is too old - removing", queuedMsgHostIter->first.c_str(), _channelName.c_str());
              delete queuedMsgIter->second;
              queuedMsgIter = queuedMsgHostIter->second.erase(queuedMsgIter);
            } else {
              queuedMsgIter++;
            }
          }
        }
        if (queuedMsgHostIter->second.empty()) {
          _queuedMessages.erase(queuedMsgHostIter++);
        } else {
          queuedMsgHostIter++;
        }
      }
    }
	}
}

/**
 * Block until we have a given number of subscribers.
 */
int ZeroMQPublisher::waitForSubscribers(int count, int timeoutMs) {
	ScopeLock lock(_mutex);
	uint64_t now = Thread::getTimeStampMs();
	while (_subscriptions.size() < (unsigned int)count) {
		_pubLock.wait(_mutex, timeoutMs);
		if (timeoutMs > 0 && Thread::getTimeStampMs() - timeoutMs > now)
			break;
	}
	return _subscriptions.size();
}

void ZeroMQPublisher::addedSubscriber(const string remoteId, const string subId) {
	ScopeLock lock(_mutex);

	// ZeroMQPublisher::run calls us without a remoteId
	if (remoteId.length() != 0) {
		// we already know about this subscription
		if (_pendingSubscriptions.find(subId) != _pendingSubscriptions.end())
			return;
		_pendingSubscriptions[subId] = remoteId;
	}

	// if we received a subscription from xpub and the node socket
	if (_pendingSubscriptions.find(subId) == _pendingSubscriptions.end() ||
	        _pendingZMQSubscriptions.find(subId) == _pendingZMQSubscriptions.end()) {
		return;
	}

	_subscriptions[subId] = _pendingSubscriptions[subId];
	_pendingSubscriptions.erase(subId);
	_pendingZMQSubscriptions.erase(subId);

	_subUUIDs.insert(subId);

	if (_greeter != NULL)
		_greeter->welcome((Publisher*)_facade, _pendingSubscriptions[subId], subId);
	UMUNDO_SIGNAL(_pubLock);
}

void ZeroMQPublisher::removedSubscriber(const string remoteId, const string subId) {
	ScopeLock lock(_mutex);

	if (_subscriptions.find(subId) == _subscriptions.end())
		return;

	_subscriptions.erase(subId);
	_subUUIDs.erase(subId);

	if (_greeter != NULL)
		_greeter->farewell((Publisher*)_facade, _pendingSubscriptions[subId], subId);
	UMUNDO_SIGNAL(_pubLock);
}

void ZeroMQPublisher::send(Message* msg) {
  ScopeLock lock(_mutex);

  //LOG_DEBUG("ZeroMQPublisher sending msg on %s", _channelName.c_str());
	if (_isSuspended) {
		LOG_WARN("Not sending message on suspended publisher");
		return;
	}

	// topic name or explicit subscriber id is first message in envelope
	zmq_msg_t channelEnvlp;
	if (msg->getMeta().find("um.sub") != msg->getMeta().end()) {
		// explicit destination
		if (_subUUIDs.find(msg->getMeta("um.sub")) == _subUUIDs.end() && !msg->isQueued()) {
			LOG_INFO("Subscriber %s is not (yet) connected on %s - queuing message", msg->getMeta("um.sub").c_str(), _channelName.c_str());
      Message* queuedMsg = new Message(*msg); // copy message
      queuedMsg->setQueued(true);
      _queuedMessages[msg->getMeta("um.sub")].push_back(std::make_pair(Thread::getTimeStampMs(), queuedMsg));
      return;
    }
		ZMQ_PREPARE_STRING(channelEnvlp, msg->getMeta("um.sub").c_str(), msg->getMeta("um.sub").size());
	} else {
		// everyone on channel
		ZMQ_PREPARE_STRING(channelEnvlp, _channelName.c_str(), _channelName.size());
	}

	// mandatory meta fields
	msg->putMeta("um.pub", _uuid);
	msg->putMeta("um.proc", procUUID);
	msg->putMeta("um.host", Host::getHostId());

	map<string, string>::const_iterator metaIter = _mandatoryMeta.begin();
	while(metaIter != _mandatoryMeta.end()) {
		if (metaIter->second.length() > 0)
			msg->putMeta(metaIter->first, metaIter->second);
		metaIter++;
	}
  
	zmq_sendmsg(_socket, &channelEnvlp, ZMQ_SNDMORE) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&channelEnvlp) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

	// all our meta information
	for (metaIter = msg->getMeta().begin(); metaIter != msg->getMeta().end(); metaIter++) {
		// string key(metaIter->first);
		// string value(metaIter->second);
		// std::cout << key << ": " << value << std::endl;

		// string length of key + value + two null bytes as string delimiters
		size_t metaSize = (metaIter->first).length() + (metaIter->second).length() + 2;
		zmq_msg_t metaMsg;
		ZMQ_PREPARE(metaMsg, metaSize);

		char* writePtr = (char*)zmq_msg_data(&metaMsg);

		memcpy(writePtr, (metaIter->first).data(), (metaIter->first).length());
		// indexes start at zero, so length is the byte after the string
		((char*)zmq_msg_data(&metaMsg))[(metaIter->first).length()] = '\0';
		assert(strlen((char*)zmq_msg_data(&metaMsg)) == (metaIter->first).length());
		assert(strlen(writePtr) == (metaIter->first).length()); // just to be sure

		// increment write pointer
		writePtr += (metaIter->first).length() + 1;

		memcpy(writePtr,
		       (metaIter->second).data(),
		       (metaIter->second).length());
		// first string + null byte + second string
		((char*)zmq_msg_data(&metaMsg))[(metaIter->first).length() + 1 + (metaIter->second).length()] = '\0';
		assert(strlen(writePtr) == (metaIter->second).length());

		zmq_sendmsg(_socket, &metaMsg, ZMQ_SNDMORE) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
		zmq_msg_close(&metaMsg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

	}

	// data as the second part of a multipart message
	zmq_msg_t publication;
	ZMQ_PREPARE_DATA(publication, msg->data(), msg->size());
	zmq_sendmsg(_socket, &publication, 0) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&publication) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}


}