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
#include "umundo/Message.h"
#include "umundo/UUID.h"

// include order matters with MSVC ...
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

#include "umundo/config.h"
#if defined UNIX || defined IOS || defined IOSSIM
#include <stdio.h> // snprintf
#endif

namespace umundo {

ZeroMQSubscriber::ZeroMQSubscriber() {}

void ZeroMQSubscriber::init(const Options* config) {

	(_subSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_SUB))     || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_readOpSocket  = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_writeOpSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_bind(_readOpSocket, readOpId.c_str())  && UM_LOG_WARN("zmq_bind: %s", zmq_strerror(errno));
	zmq_connect(_writeOpSocket, readOpId.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	assert(_channelName.size() > 0);

	int hwm = NET_ZEROMQ_RCV_HWM;
	std::string subId("um.sub." + _uuid);
	std::string lastSub("~" + _uuid); ///< this needs to have very "late" alphabetical order to ensure all channels are subscribed to first

//	zmq_setsockopt(_subSocket, ZMQ_IDENTITY, subId.c_str(), subId.length()) && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, _channelName.c_str(), _channelName.length())  && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, lastSub.c_str(), lastSub.length())  && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));

	int rcvTimeOut = 30;
	zmq_setsockopt(_subSocket, ZMQ_RCVTIMEO, &rcvTimeOut, sizeof(rcvTimeOut));

	// reconnection intervals
#if 0
	int reconnect_ivl_min = 100;
	int reconnect_ivl_max = 200;
	zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL, &reconnect_ivl_min, sizeof(int)) && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL_MAX, &reconnect_ivl_max, sizeof(int)) && UM_LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
#endif

	zmq_bind(_subSocket, std::string("inproc://" + subId).c_str());
}

ZeroMQSubscriber::~ZeroMQSubscriber() {
	UM_LOG_INFO("deleting subscriber for %s", _channelName.c_str());
	stop();

	char tmp[4];
	zmq_send(_writeOpSocket, tmp, 4, 0) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno)); // unblock poll
	join(); // wait for thread to finish

	std::string subId("um.sub." + _uuid);
	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_disconnect(_writeOpSocket, readOpId.c_str()) && UM_LOG_ERR("zmq_disconnect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	// not needed for inproc?
//	zmq_unbind(_readOpSocket, readOpId.c_str()) && UM_LOG_WARN("zmq_unbind: %s", zmq_strerror(errno));
//	zmq_unbind(_subSocket, std::string("inproc://" + subId).c_str()) && UM_LOG_WARN("zmq_unbind: %s", zmq_strerror(errno));

	zmq_close(_subSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_readOpSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_writeOpSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
}

SharedPtr<Implementation> ZeroMQSubscriber::create() {
	return SharedPtr<ZeroMQSubscriber>(new ZeroMQSubscriber());
}

void ZeroMQSubscriber::suspend() {
	RScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;

};

void ZeroMQSubscriber::resume() {
	RScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;
};

void ZeroMQSubscriber::added(const PublisherStub& pub, const NodeStub& node) {
	RScopeLock lock(_mutex);

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

		UM_LOG_INFO("%s subscribing to %s on %s", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ss.str().c_str());

		if (isStarted()) {
			ZMQ_INTERNAL_SEND("connectPub", ss.str().c_str());
		} else {
			zmq_connect(_subSocket, ss.str().c_str()) && UM_LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));
		}
	}

	_pubs[pub.getUUID()] = pub;
	_domainPubs.insert(std::make_pair(pub.getDomain(), pub.getUUID()));
}

void ZeroMQSubscriber::removed(const PublisherStub& pub, const NodeStub& node) {
	RScopeLock lock(_mutex);

	// TODO: This fails for publishers added via different nodes
	if (_pubs.find(pub.getUUID()) != _pubs.end())
		_pubs.erase(pub.getUUID());

	if (_domainPubs.count(pub.getDomain()) == 0)
		return;

	std::multimap<std::string, std::string>::iterator domIter = _domainPubs.find(pub.getDomain());

	while(domIter != _domainPubs.end()) {
		if (domIter->second == pub.getUUID()) {
			_domainPubs.erase(domIter++);
		} else {
			domIter++;
		}
	}

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

		UM_LOG_INFO("%s unsubscribing from %s on %s", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ss.str().c_str());

		if (isStarted()) {
			ZMQ_INTERNAL_SEND("disconnectPub", ss.str().c_str());
		} else {
			zmq_disconnect(_subSocket, ss.str().c_str()) && UM_LOG_ERR("zmq_disconnect %s: %s", ss.str().c_str(), zmq_strerror(errno));
		}
	}
}

void ZeroMQSubscriber::setReceiver(Receiver* receiver) {
	stop();
	ZMQ_INTERNAL_SEND("",""); // just unblock
	join();
	_receiver = receiver;
	if (_receiver != NULL) {
		start();
	} else {
		UM_LOG_INFO("Unsetting receiver - subscriber stopped");
	}
}

void ZeroMQSubscriber::run() {
	zmq_pollitem_t items [] = {
		{ _readOpSocket, 0, ZMQ_POLLIN, 0 }, // one of our members wants to manipulate a socket
		{ _subSocket,    0, ZMQ_POLLIN, 0 }, // publication requests
	};

	while(isStarted()) {
		int rc = zmq_poll(items, 2, -1);
		if (rc < 0) {
			UM_LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
		}

		if (!isStarted())
			return;

		if (items[1].revents & ZMQ_POLLIN && _receiver != NULL) {
			Message* msg = getNextMsg();
			if (msg) {
				_receiver->receive(msg);
				delete msg;
			}
		}

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
					zmq_connect(_subSocket, endpoint) && UM_LOG_ERR("zmq_connect %s: %s", endpoint, zmq_strerror(errno));
				} else if (strcmp(op, "disconnectPub") == 0) {
					zmq_disconnect(_subSocket, endpoint) && UM_LOG_ERR("zmq_disconnect %s: %s", endpoint, zmq_strerror(errno));
				}

				zmq_getsockopt (_readOpSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&message);
				zmq_msg_close (&endpointMsg);

				assert(!more); // we read all messages
				if (!more)
					break;      //  Last message part
			}
		}

	}
}

Message* ZeroMQSubscriber::getNextMsg() {
	int32_t more;
	size_t more_size = sizeof(more);
	bool readChannelName = false;

	Message* msg = new Message();
	while (1) {
		// read the whole message
		zmq_msg_t message;
		zmq_msg_init(&message) && UM_LOG_WARN("zmq_msg_init: %s",zmq_strerror(errno));

		int rc;
		rc = zmq_recvmsg(_subSocket, &message, 0);
		if (rc < 0) {
			UM_LOG_WARN("zmq_recvmsg: %s",zmq_strerror(errno));
			zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
			delete msg;
			return NULL;
		}

		size_t msgSize = zmq_msg_size(&message);
		char* msgData = (char*)zmq_msg_data(&message);
		zmq_getsockopt(_subSocket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_WARN("zmq_getsockopt: %s",zmq_strerror(errno));

		// is this the first message with the channelname?
		if (!readChannelName) {
			if (memchr(msgData, 0, msgSize) == msgData + (msgSize - 1)) {
				msg->putMeta("um.channel", std::string(msgData, msgSize - 1));
				zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
				readChannelName = true;
				continue;
			} else {
				UM_LOG_ERR("Subscriber on channel %s received gibberish", _channelName.c_str());
				delete msg;
				return NULL;
			}
		}

        const char* readPtr = msgData;
        size_t remainingSize = msgSize;

        while(true) {
            // Message Version
            uint8_t msgVersion = 0;
            if (remainingSize > 0) {
                readPtr = Message::read(readPtr, &msgVersion);
                remainingSize -= 1;
            } else {
                UM_LOG_ERR("Subscriber on channel %s received empty message", _channelName.c_str());
                delete msg;
                return NULL;
            }
            
            if (remainingSize > 0) {
                switch (msgVersion) {
                case Message::UM_MSG_VERSION_01: {
                    
                    // publisher identity
                    std::string pubUUID;
                    readPtr = UUID::readBinToHex(readPtr, pubUUID);
                    remainingSize -= 16;

                    if (!UUID::isUUID(pubUUID)) {
                        UM_LOG_ERR("Subscriber on channel %s received gibberish", _channelName.c_str());
                        zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
                        delete msg;
                        return NULL;
                    }
                    
                    // read second byte with header flags
                    uint8_t headerFlags;
                    readPtr = Message::read(readPtr, &headerFlags);
                    remainingSize -= 1;
                    
                    
                    // read next byte++ with the header length
                    uint64_t headerSize = 0;
                    readPtr = Message::readCompact(readPtr, &headerSize, remainingSize);
                    if (readPtr == 0) {
                        UM_LOG_ERR("Subscriber on channel %s received gibberish", _channelName.c_str());
                        zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
                        delete msg;
                        return NULL;
                    }
                    remainingSize -= 1;
                    
                    if (headerSize > remainingSize) {
                        UM_LOG_ERR("Subscriber on channel %s received wrong header size", _channelName.c_str());
                        zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
                        delete msg;
                        return NULL;
                    }
                    
                    const char* headerData = readPtr;
                    
                    uint64_t payloadSize = (remainingSize - headerSize);
                    const char* payloadData = headerData + headerSize;
                    
                    remainingSize -= headerSize;
                    remainingSize -= payloadSize;
                    assert(remainingSize == 0);
                    
                    if (headerFlags & Message::UM_COMPR_MSG) {

                        if (headerFlags & Message::UM_COMPR_KEYFRAME) {
                            if (_pubComprCtx.find(pubUUID) != _pubComprCtx.end()) {
                                Message::freeCompression(_pubComprCtx[pubUUID]);
                            }
                            _pubComprCtx[pubUUID] = Message::createCompression();
                        } else {
                            if (_pubComprCtx.find(pubUUID) == _pubComprCtx.end()) {
                                UM_LOG_ERR("Subscriber on channel %s waiting for keyframe", _channelName.c_str());
                                zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
                                delete msg;
                                return NULL;
                            }
                        }
                        
                        void* ctx = _pubComprCtx[pubUUID];
                        if (headerFlags & Message::UM_COMPR_LZ4) {
                            if (headerSize > 0)
                                msg->uncompress("lz4", ctx, headerData, headerSize, Message::HEADER);
                            if (payloadSize > 0)
                                msg->uncompress("lz4", ctx, payloadData, payloadSize, Message::PAYLOAD);
                        }
                    } else {
                        // uncompressed, just read into the message
                        msg->setData(payloadData, payloadSize);
                        msg->readHeaders(headerData, headerSize);
                    }
                    
                    zmq_msg_close(&message) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
                    goto MESSAGE_READ;

                }
                        
                    
                default:
                    UM_LOG_ERR("Unsupported message version %d", msgVersion);
                    delete msg;
                    return NULL;
                }
            } else {
                UM_LOG_ERR("Subscriber on channel %s received gibberish", _channelName.c_str());
                delete msg;
                return NULL;
            }
        }
	}
MESSAGE_READ:
    zmq_getsockopt(_subSocket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_WARN("zmq_getsockopt: %s",zmq_strerror(errno));
    if (more) {
        UM_LOG_ERR("Subscriber on channel %s received wrong message format", _channelName.c_str());
        delete msg;
        return NULL;
    }

    return msg;
}

bool ZeroMQSubscriber::hasNextMsg() {
	zmq_pollitem_t items[1];
	items[0].socket = _subSocket;
	items[0].events = ZMQ_POLLIN;

	int rc = zmq_poll(items, 1, 0);
	if (rc < 0) {
		UM_LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
		return false;
	}
	if (items[0].revents & ZMQ_POLLIN) {
		return true;
	}
	return false;
}


}
