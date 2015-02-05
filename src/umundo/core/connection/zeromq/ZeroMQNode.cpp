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

#define UMUNDO_PERF_WINDOW_LENGTH_MS 5000
#define UMUNDO_PERF_BUCKET_LENGTH_MS 200.0

#include "umundo/core/connection/zeromq/ZeroMQNode.h"
#include "umundo/core/discovery/Discovery.h"

#include "umundo/config.h"

#if defined UNIX || defined IOS || defined IOSSIM
#include <arpa/inet.h> // htons
#include <string.h> // strlen, memcpy
#include <stdio.h> // snprintf

#include <sys/socket.h>
#include <netdb.h>

#endif

#ifdef WIN32
#include <WS2tcpip.h>
#endif

#include <boost/lexical_cast.hpp>

#include "umundo/core/Message.h"
#include "umundo/core/UUID.h"
#include "umundo/core/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/core/connection/zeromq/ZeroMQSubscriber.h"

#include <math.h>       /* round */

/**
 * Drain any superfluous messages in an envelope
 */
#define DRAIN_SOCKET(socket) \
for(;;) { \
	zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &moreSize) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno)); \
	if (!more) \
		break; \
	UM_LOG_INFO("Superfluous message on " #socket ""); \
	zmq_recv(socket, NULL, 0, 0) && UM_LOG_ERR("zmq_recv: %s", zmq_strerror(errno)); \
}

/**
 * Some default declarations to work with messages
 */
#define COMMON_VARS \
int more; (void)more;\
size_t moreSize = sizeof(more); (void)moreSize;\
int msgSize; (void)msgSize;\
char* recvBuffer; (void)recvBuffer;\
const char* readPtr; (void)readPtr;\
char* writeBuffer; (void)writeBuffer;\
char* writePtr; (void)writePtr;

/**
 * Allocate new ZMQ message with given size and set writeBuffer and writePtr
 */
#define PREPARE_MSG(msg, size) \
zmq_msg_t msg; \
zmq_msg_init(&msg) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno));\
writeBuffer = (char*)zmq_msg_data(&msg);\
writePtr = writeBuffer;\
 
/**
 * Receive a message from given socket and set readBuffer and readPtr
 */
#define RECV_MSG(socket, msg) \
zmq_msg_t msg; \
zmq_msg_init(&msg) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_recvmsg(socket, &msg, 0) == -1 && UM_LOG_ERR("zmq_recvmsg: %s", zmq_strerror(errno)); \
msgSize = zmq_msg_size(&msg); \
recvBuffer = (char*)zmq_msg_data(&msg); \
readPtr = recvBuffer;

#define ZMQ_ADDRESS(ep) \
ep.getTransport() + "://" + ((ep.getIP().size() == 0 || ep.getIP() == "*") ? "localhost" : ep.getIP()) + ":" + toStr(ep.getPort())

#define REMAINING_BYTES_TOREAD (msgSize - (readPtr - recvBuffer))

#define PUB_INFO_SIZE(pub) \
pub.getChannelName().length() + 1 + /* channelName and terminator */ \
pub.getUUID().length() + 1 +        /* UUID and terminator */ \
sizeof(uint16_t) +                  /* type: ZMQ, RTP */ \
sizeof(uint16_t)                    /* port of publisher */

#define SUB_INFO_SIZE(sub) \
sub.getChannelName().length() + 1 + /* channelName and terminator */ \
sub.getUUID().length() + 1 +        /* channelNam and terminator */ \
sizeof(uint16_t) +                  /* type: ZMQ, RTP */ \
sizeof(uint16_t) +                  /* whether this is multicats */ \
sub.getIP().length() + 1 +          /* IP address */ \
sizeof(uint16_t)                    /* port of subscriber */

#define NODE_BROADCAST_MSG(msg) \
std::map<std::string, NodeStub>::iterator nodeIter_ = _connFrom.begin();\
while (nodeIter_ != _connFrom.end()) {\
	zmq_msg_t broadCastMsgCopy_;\
	zmq_msg_init(&broadCastMsgCopy_) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno));\
	zmq_msg_copy(&broadCastMsgCopy_, &msg) && UM_LOG_ERR("zmq_msg_copy: %s", zmq_strerror(errno));\
	UM_LOG_DEBUG("%s: Broadcasting to %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(nodeIter_->first).c_str()); \
	zmq_send(_nodeSocket, nodeIter_->first.c_str(), nodeIter_->first.length(), ZMQ_SNDMORE | ZMQ_DONTWAIT); \
	zmq_msg_send(&broadCastMsgCopy_, _nodeSocket, ZMQ_DONTWAIT); \
	zmq_msg_close(&broadCastMsgCopy_) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));\
	nodeIter_++;\
}

#define ASSERT_BYTES_WRITTEN(BYTES) assert(writePtr - writeBuffer == BYTES)

#define SEND_DEBUG_ENVELOPE(msg)\
zmq_send(_nodeSocket, msg.c_str(), msg.length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));


namespace umundo {

void* ZeroMQNode::getZeroMQContext() {
	if (_zmqContext == NULL) {
		(_zmqContext = zmq_ctx_new()) || UM_LOG_ERR("zmq_init: %s",zmq_strerror(errno));
	}
	return _zmqContext;
}
void* ZeroMQNode::_zmqContext = NULL;

ZeroMQNode::ZeroMQNode() {
}

ZeroMQNode::~ZeroMQNode() {
	UM_TRACE("~ZeroMQNode");
	stop();

	UM_LOG_INFO("%s: node shutting down", SHORT_UUID(_uuid).c_str());

	char tmp[4];
	writeVersionAndType(tmp, Message::UM_SHUTDOWN);
	zmq_send(_writeOpSocket, tmp, 4, 0) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno)); // unblock poll
	join(); // wait for thread to finish

	COMMON_VARS;
	RScopeLock lock(_mutex);

	PREPARE_MSG(shutdownMsg, 4 + 37);
	writePtr = writeVersionAndType(writePtr, Message::UM_SHUTDOWN);
	assert(writePtr - writeBuffer == 4);
	writePtr = Message::write(writePtr, _uuid);
	assert(writePtr - writeBuffer == zmq_msg_size(&shutdownMsg));

	NODE_BROADCAST_MSG(shutdownMsg);
	zmq_msg_close(&shutdownMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

	// delete all connection information
	while(_connTo.size() > 0) {
		NodeStub node = _connTo.begin()->second->node;
		if(node) {
			removed(node);
			receivedInternalOp();
		} else {
			_connTo.erase(_connTo.begin());
		}
	}

	while(_connFrom.size() > 0) {
		_connFrom.erase(_connFrom.begin());
	}

	if (_sockets != NULL)
		free(_sockets);

	// close sockets
	zmq_close(_nodeSocket)    && UM_LOG_ERR("zmq_close: %s", zmq_strerror(errno));
	zmq_close(_pubSocket)     && UM_LOG_ERR("zmq_close: %s", zmq_strerror(errno));
	zmq_close(_subSocket)     && UM_LOG_ERR("zmq_close: %s", zmq_strerror(errno));
	zmq_close(_readOpSocket)  && UM_LOG_ERR("zmq_close: %s", zmq_strerror(errno));
	zmq_close(_writeOpSocket) && UM_LOG_ERR("zmq_close: %s", zmq_strerror(errno));
	UM_LOG_INFO("%s: node gone", SHORT_UUID(_uuid).c_str());

}

void ZeroMQNode::init(const Options* options) {
	UM_TRACE("init");

	_options = options->getKVPs();
	_port = strTo<uint16_t>(_options["endpoint.port"]);
	_pubPort = strTo<uint16_t>(_options["node.port.pub"]);
	_allowLocalConns = strTo<bool>(_options["node.allowLocal"]);

	_transport = "tcp";
	_ip = _options["endpoint.ip"];
	_lastNodeInfoBroadCast = Thread::getTimeStampMs();

	int routMand = 1;
	int routProbe = 0;
	int vbsSub = 1;
	int sndhwm = NET_ZEROMQ_SND_HWM;
	int rcvhwm = NET_ZEROMQ_RCV_HWM;

	(_nodeSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_ROUTER))  || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_pubSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_XPUB))     || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_subSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_SUB))      || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_readOpSocket  = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR)) || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_writeOpSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR)) || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	// connect read and write internal op sockets
	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_bind(_readOpSocket, readOpId.c_str())  && UM_LOG_ERR("zmq_bind: %s", zmq_strerror(errno));
	zmq_connect(_writeOpSocket, readOpId.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	// connect node socket
	if (_port > 0) {
		std::stringstream ssNodeAddress;
		ssNodeAddress << "tcp://" << _ip << ":" << _port;
		zmq_bind(_nodeSocket, ssNodeAddress.str().c_str()) && UM_LOG_ERR("zmq_bind: %s", zmq_strerror(errno));
		UM_LOG_DEBUG("zmq_bind: Listening on %s", ssNodeAddress.str().c_str());
	} else {
		_port = bindToFreePort(_nodeSocket, "tcp", _ip);
		UM_LOG_DEBUG("zmq_bind: Listening on %s://%s:%d", "tcp", _ip.c_str(), _port);

	}

	std::string nodeId("um.node." + _uuid);
	zmq_bind(_nodeSocket, std::string("inproc://" + nodeId).c_str()) && UM_LOG_ERR("zmq_bind: %s", zmq_strerror(errno));
	//  zmq_bind(_nodeSocket, std::string("ipc://" + nodeId).c_str())    && UM_LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + nodeId).c_str(),  zmq_strerror(errno));

	// connect publisher socket
	if (_pubPort > 0) {
		std::stringstream ssPubAddress;
		ssPubAddress << "tcp://" << _ip << ":" << _pubPort;
		zmq_bind(_pubSocket, ssPubAddress.str().c_str()) && UM_LOG_ERR("zmq_bind: %s", zmq_strerror(errno));
	} else {
		_pubPort = bindToFreePort(_pubSocket, "tcp", _ip);
	}

	std::string pubId("um.pub." + _uuid);
	zmq_bind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && UM_LOG_ERR("zmq_bind: %s", zmq_strerror(errno));
	//  zmq_bind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && UM_LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + pubId).c_str(), zmq_strerror(errno));

	zmq_setsockopt(_pubSocket, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm))         && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_pubSocket, ZMQ_XPUB_VERBOSE, &vbsSub, sizeof(vbsSub))   && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno)); // receive all subscriptions

	zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &rcvhwm, sizeof(rcvhwm)) && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, "", 0)                && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno)); // subscribe to every internal publisher

	zmq_setsockopt(_nodeSocket, ZMQ_IDENTITY, _uuid.c_str(), _uuid.length())        && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_nodeSocket, ZMQ_ROUTER_MANDATORY, &routMand, sizeof(routMand))  && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_nodeSocket, ZMQ_PROBE_ROUTER, &routProbe, sizeof(routProbe))    && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));

	_dirtySockets = true;
	_sockets = NULL;
	_nrStdSockets = 4;
	_stdSockets[0].socket = _nodeSocket;
	_stdSockets[1].socket = _pubSocket;
	_stdSockets[2].socket = _readOpSocket;
	_stdSockets[3].socket = _subSocket;
	_stdSockets[0].fd = _stdSockets[1].fd = _stdSockets[2].fd = _stdSockets[3].fd = 0;
	_stdSockets[0].events = _stdSockets[1].events = _stdSockets[2].events = _stdSockets[3].events = ZMQ_POLLIN;

	start();
}

SharedPtr<Implementation> ZeroMQNode::create() {
	UM_TRACE("create");
	return SharedPtr<ZeroMQNode>(new ZeroMQNode());
}

void ZeroMQNode::suspend() {
	UM_TRACE("suspend");
}

void ZeroMQNode::resume() {
	UM_TRACE("resume");
}

uint16_t ZeroMQNode::bindToFreePort(void* socket, const std::string& transport, const std::string& address) {
	UM_TRACE("bindToFreePort");
	std::stringstream ss;
	int port = 4242;

	ss << transport << "://" << address << ":" << port;

	while(zmq_bind(socket, ss.str().c_str()) == -1) {
		switch(errno) {
		case EADDRINUSE:
			port++;
			ss.clear();        // clear error bits
			ss.str(std::string());  // reset string
			ss << transport << "://" << address << ":" << port;
			break;
		default:
			UM_LOG_WARN("zmq_bind at %s: %s", ss.str().c_str(), zmq_strerror(errno));
			Thread::sleepMs(100);
		}
	}

	return port;
}

std::map<std::string, NodeStub> ZeroMQNode::connectedFrom() {
	RScopeLock lock(_mutex);
	UM_TRACE("connectedFrom");
	return _connFrom;
}

std::map<std::string, NodeStub> ZeroMQNode::connectedTo() {
	RScopeLock lock(_mutex);
	UM_TRACE("connectedTo");

	std::map<std::string, NodeStub> to;
	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator nodeIter = _connTo.begin();
	while (_connTo.size() > 0 && nodeIter != _connTo.end()) {
		// only report UUIDs as keys
		if (UUID::isUUID(nodeIter->first)) {
			to[nodeIter->first] = nodeIter->second->node;
		}
		nodeIter++;
	}
	return to;
}

void ZeroMQNode::addSubscriber(Subscriber& sub) {
	RScopeLock lock(_mutex);
	UM_TRACE("addSubscriber");
	if (_subs.find(sub.getUUID()) != _subs.end())
		return;

	UM_LOG_INFO("%s added subscriber %s on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

	_subs[sub.getUUID()] = sub;

	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator nodeIter = _connTo.begin();
	while (nodeIter != _connTo.end()) {
		if (!UUID::isUUID(nodeIter->first)) {
			// nodes are contained twice in _connTo, per address and per uuid
			nodeIter++;
			continue;
		}
		if (nodeIter->second && nodeIter->second->node) {
			std::map<std::string, PublisherStub> pubs = nodeIter->second->node.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();

			// iterate all remote publishers and add this sub
			while (pubIter != pubs.end()) {
				if (sub.matches(pubIter->second)) {

					sub.added(pubIter->second, nodeIter->second->node);
					sendSubscribeToPublisher(nodeIter->first.c_str(), sub, pubIter->second);

				}
				pubIter++;
			}
		}
		nodeIter++;
	}
}

void ZeroMQNode::removeSubscriber(Subscriber& sub) {
	RScopeLock lock(_mutex);
	UM_TRACE("removeSubscriber");

	if (_subs.find(sub.getUUID()) == _subs.end())
		return;

	UM_LOG_INFO("%s removed subscriber %s on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator nodeIter = _connTo.begin();
	while (nodeIter != _connTo.end()) {
		if (nodeIter->second->node) {
			std::map<std::string, PublisherStub> pubs = nodeIter->second->node.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();

			// iterate all remote publishers and remove this sub
			while (pubIter != pubs.end()) {
				if(sub.matches(pubIter->second)) {
					sub.removed(pubIter->second, nodeIter->second->node);
					sendUnsubscribeFromPublisher(nodeIter->first.c_str(), sub, pubIter->second);
				}
				pubIter++;
			}
		}
		nodeIter++;
	}
	_subs.erase(sub.getUUID());
}

void ZeroMQNode::addPublisher(Publisher& pub) {
	RScopeLock lock(_mutex);
	UM_TRACE("addPublisher");
	COMMON_VARS;

	if (_pubs.find(pub.getUUID()) != _pubs.end())
		return;

	size_t bufferSize = 4 + _uuid.length() + 1 + PUB_INFO_SIZE(pub);
	PREPARE_MSG(pubAddedMsg, bufferSize);

	UM_LOG_INFO("%s added publisher %s on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

	writePtr = writeVersionAndType(writePtr, Message::UM_PUB_ADDED);
	writePtr = Message::write(writePtr, _uuid);
	writePtr = write(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&pubAddedMsg, _writeOpSocket, 0) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));

	_pubs[pub.getUUID()] = pub;
	zmq_msg_close(&pubAddedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

}

void ZeroMQNode::removePublisher(Publisher& pub) {
	RScopeLock lock(_mutex);
	UM_TRACE("removePublisher");
	COMMON_VARS;

	if (_pubs.find(pub.getUUID()) == _pubs.end())
		return;

	size_t bufferSize = 4 + _uuid.length() + 1 + PUB_INFO_SIZE(pub);
	PREPARE_MSG(pubRemovedMsg, bufferSize);

	UM_LOG_INFO("%s removed publisher %s on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

	writePtr = writeVersionAndType(writePtr, Message::UM_PUB_REMOVED);
	writePtr = Message::write(writePtr, _uuid);
	writePtr = write(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&pubRemovedMsg, _writeOpSocket, 0) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}

	zmq_msg_close(&pubRemovedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
	_pubs.erase(pub.getUUID());

}

/**
 *
 * Information about other nodes receives via two different interfaces:
 * 1. Someone in our runtime calls node.add(endpoint), which ends up here
 * 2. A remote node sends a UM_CONNECT_REQ
 *
 * Usually both happen in short succession, but that must not necessarily be the
 * case. Variant 1. is usually performed by an application developer or the
 * discovery layer. Variant 2. originates from remote nodes for which 1. happened.
 *
 * If 1. is performed, but 2. never happens, we are half-subscribed, that is, our
 * subscribers will connect to the other nodes publishers, but not vice versa.
 * These two modes are reflected as entries in _connTo and _connFrom respectively.
 *
 */

void ZeroMQNode::added(ENDPOINT_RS_TYPE endPoint) {
	RScopeLock lock(_mutex);
	UM_TRACE("added");

	if (endPoint.getUUID() == _uuid) // that's us!
		return;

	std::string zmqAdress = ZMQ_ADDRESS(endPoint);

	if (_endPoints.find(zmqAdress) != _endPoints.end()) {

		UM_LOG_INFO("%s: Already aware of endpoint at %s - increasing reference count to %d",
		            SHORT_UUID(_uuid).c_str(),
		            zmqAdress.c_str(),
		            _endPoints[zmqAdress].size() + 1);

	} else {

		UM_LOG_INFO("%s: Adding endpoint at %s",
		            SHORT_UUID(_uuid).c_str(),
		            zmqAdress.c_str());

		COMMON_VARS;
		std::string otherAddress = ZMQ_ADDRESS(endPoint);
		// TODO: assert valid syntax!

		// write connection request to operation socket
		msgSize = 4 + zmqAdress.length() + 1;
		PREPARE_MSG(addEndPointMsg, msgSize);

		writePtr = writeVersionAndType(writePtr, Message::UM_CONNECT_REQ);
		ASSERT_BYTES_WRITTEN(4);
		writePtr = Message::write(writePtr, zmqAdress);
		ASSERT_BYTES_WRITTEN(msgSize);

		zmq_sendmsg(_writeOpSocket, &addEndPointMsg, 0) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
		zmq_msg_close(&addEndPointMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
	}

	// remember to handle disconnects later
	_endPoints[zmqAdress].insert(endPoint);

}

void ZeroMQNode::removed(ENDPOINT_RS_TYPE endPoint) {
	RScopeLock lock(_mutex);
	UM_TRACE("removed");

	if (endPoint.getUUID() == _uuid) // that's us!
		return;

	std::string zmqAdress = ZMQ_ADDRESS(endPoint);

	if (_endPoints.find(zmqAdress) == _endPoints.end()) {
		UM_LOG_INFO("%s: Not removing endpoint at %s - none known at address",
		            SHORT_UUID(_uuid).c_str(), zmqAdress.c_str());
		return;
	}

	if (_endPoints[zmqAdress].find(endPoint) == _endPoints[zmqAdress].end()) {
		UM_LOG_INFO("%s: Not removing endpoint %s - not known",
		            SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str());
		return;
	}

	_endPoints[zmqAdress].erase(endPoint);
	UM_LOG_INFO("%s: Removing endpoint at %s - decreasing reference count to %d",
	            SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str(), _endPoints[zmqAdress].size());

	if (_endPoints[zmqAdress].size() > 0)
		return;

	UM_LOG_INFO("%s: No more endpoints at %s - disconnecting",
	            SHORT_UUID(_uuid).c_str(), zmqAdress.c_str());

	// get rid of key!
	_endPoints.erase(zmqAdress);

	COMMON_VARS;

	msgSize = 4 + zmqAdress.length() + 1;
	PREPARE_MSG(removeEndPointMsg, msgSize);

	writePtr = writeVersionAndType(writePtr, ZeroMQNode::UM_DISCONNECT);
	ASSERT_BYTES_WRITTEN(4);
	writePtr = Message::write(writePtr, zmqAdress);
	ASSERT_BYTES_WRITTEN(msgSize);

	zmq_sendmsg(_writeOpSocket, &removeEndPointMsg, 0) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
	zmq_msg_close(&removeEndPointMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

void ZeroMQNode::changed(ENDPOINT_RS_TYPE endPoint, uint64_t what) {
	if (what & Discovery::IFACE_REMOVED) {
		UM_LOG_INFO("%s: gone on some interface -> removing and readding endpoint (be more clever here)", SHORT_UUID(_uuid).c_str());
		removed(endPoint);
		added(endPoint);
	} else {
		UM_LOG_INFO("%s: found %s on new interface, ignoring", SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str());
	}
}

/**
 * Process messages sent to our external _nodeSocket. Will handle:
 *
 * UM_DEBUG
 * UM_CONNECT_REQ
 * UM_SUBSCRIBE
 * UM_UNSUBSCRIBE
 *
 */
void ZeroMQNode::receivedFromNodeSocket() {
	UM_TRACE("receivedFromNodeSocket");
	COMMON_VARS;

	// read first message
	RECV_MSG(_nodeSocket, header);
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgRcvd++;
		_buckets.back().sizeMetaMsgRcvd += msgSize;
	}

	std::string from(recvBuffer, msgSize);
	zmq_msg_close(&header) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

	if (from.length() != 36) {
		UM_LOG_WARN("%s: Got unprefixed message on _nodeSocket - discarding", SHORT_UUID(_uuid).c_str());
		return;
	}

	RECV_MSG(_nodeSocket, content);

	// remember node and update last seen
	//touchNeighbor(from);

	// dealer socket sends no delimiter, but req does
	if (REMAINING_BYTES_TOREAD == 0) {
		zmq_getsockopt(_nodeSocket, ZMQ_RCVMORE, &more, &moreSize) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
		if (!more) {
			zmq_msg_close(&content) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
			return;
		}
		RECV_MSG(_nodeSocket, content);
	}

	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgRcvd++;
		_buckets.back().sizeMetaMsgRcvd += msgSize;
	}

	// assume the mesage has at least version and type
	if (REMAINING_BYTES_TOREAD < 4) {
		UM_LOG_WARN("%s: node socket received gibberish without version or type", SHORT_UUID(_uuid).c_str());
		zmq_msg_close(&content) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		return;
	}

	uint16_t type;
	uint16_t version;
	readPtr = readVersionAndType(recvBuffer, version, type);

	if (version != Message::UM_VERSION) {
		UM_LOG_WARN("%s: node socket received unversioned or different message format version - discarding", SHORT_UUID(_uuid).c_str());
		zmq_msg_close(&content) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		return;
	}

	UM_LOG_INFO("%s: node socket received %s", SHORT_UUID(_uuid).c_str(), Message::typeToString(type));
	switch (type) {
	case Message::UM_DEBUG: {

		// someone wants debug info from us
		replyWithDebugInfo(from);
		break;
	}
	case Message::UM_CONNECT_REQ: {

		// someone is about to connect to us
		if (from != _uuid || _allowLocalConns) {
			if (_connTo.find(from) != _connTo.end()) {
				_connFrom[from] = _connTo[from]->node;
			} else {
				_connFrom[from] = NodeStub(SharedPtr<NodeStubImpl>(new NodeStubImpl()));
				_connFrom[from].getImpl()->setUUID(from);
			}

			// in any case, mark as connected from and update last seen
			_connFrom[from].updateLastSeen();
		}

		// reply with our uuid and publishers
		UM_LOG_INFO("%s: Replying with CONNECT_REP and %d pubs on _nodeSocket to %s", SHORT_UUID(_uuid).c_str(), _pubs.size(), SHORT_UUID(from).c_str());
		zmq_send(_nodeSocket, from.c_str(), from.length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno)); // return to sender
		if (_buckets.size() > 0) {
			_buckets.back().nrMetaMsgSent++;
			_buckets.back().sizeMetaMsgSent += from.length();
		}

		zmq_msg_t replyNodeInfoMsg;
		writeNodeInfo(&replyNodeInfoMsg, Message::UM_CONNECT_REP);

		zmq_sendmsg(_nodeSocket, &replyNodeInfoMsg, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
		if (_buckets.size() > 0) {
			_buckets.back().nrMetaMsgSent++;
			_buckets.back().sizeMetaMsgSent += zmq_msg_size(&replyNodeInfoMsg);
		}
		zmq_msg_close(&replyNodeInfoMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		break;
	}
	case Message::UM_SUBSCRIBE:
	case Message::UM_UNSUBSCRIBE: {
		// a remote node subscribed or unsubscribed to one of our publishers

		SharedPtr<PublisherStubImpl> pubImpl = SharedPtr<PublisherStubImpl>(new PublisherStubImpl());
		SharedPtr<SubscriberStubImpl> subImpl = SharedPtr<SubscriberStubImpl>(new SubscriberStubImpl());

		readPtr = read(readPtr, subImpl.get(), REMAINING_BYTES_TOREAD);
		readPtr = read(readPtr, pubImpl.get(), REMAINING_BYTES_TOREAD);

		std::string pubUUID = pubImpl->getUUID();
		std::string subUUID = subImpl->getUUID();
		std::string address;

		assert(REMAINING_BYTES_TOREAD == 0);

		{
			int srcFd = zmq_msg_get(&content, ZMQ_SRCFD);

			if (srcFd > 0) {
				int rc;
				(void)rc; // surpress unused warning without assert
				struct sockaddr_storage ss;
				socklen_t addrlen = sizeof ss;
				rc = getpeername (srcFd, (struct sockaddr*) &ss, &addrlen);

				char host [NI_MAXHOST];
				rc = getnameinfo ((struct sockaddr*) &ss, addrlen, host, sizeof host, NULL, 0, NI_NUMERICHOST);
				address = host;
			}
		}

		if (_pubs.find(pubUUID) == _pubs.end())
			break;

		if (type == Message::UM_SUBSCRIBE) {
			// confirm subscription
			if (!_subscriptions[subUUID].subStub) {
				_subscriptions[subUUID].subStub = SubscriberStub(subImpl);
			}
			_subscriptions[subUUID].nodeUUID = from;
			_subscriptions[subUUID].address = address;
			_subscriptions[subUUID].pending[pubUUID] = _pubs[pubUUID];

			confirmSubscription(subUUID);

		} else {
			// remove a subscription
			Subscription& confSub = _subscriptions[subUUID];
			if (_connFrom.find(confSub.nodeUUID) != _connFrom.end() && _connFrom[confSub.nodeUUID])
				_connFrom[confSub.nodeUUID].removeSubscriber(confSub.subStub);

			// move all pending subscriptions to confirmed
			std::map<std::string, Publisher>::iterator confPubIter = confSub.confirmed.begin();
			while(confPubIter != confSub.confirmed.end()) {
				confPubIter->second.removed(confSub.subStub, _connFrom[confSub.nodeUUID]);
				confPubIter++;
			}
			confSub.confirmed.clear();

		}
		break;
	}

	default:
		break;
	}
	zmq_msg_close(&content) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
	return;

}

void ZeroMQNode::receivedFromPubSocket() {
	UM_TRACE("receivedFromPubSocket");
	COMMON_VARS;
	/**
	 * someone subscribed, process here to avoid
	 * XPUB socket and thread at publisher
	 */
	zmq_msg_t message;
	while (1) {
		//  Process all parts of the message
		zmq_msg_init(&message)  && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno));
		zmq_msg_recv(&message, _pubSocket, 0);

		char* data = (char*)zmq_msg_data(&message);
		bool subscription = (data[0] == 0x1);
		std::string subChannel(data+1, zmq_msg_size(&message) - 1);
		std::string subUUID;
		// this is where we abuse alphabetic order to get the subscribers uuid after the channel subscription
		if (UUID::isUUID(subChannel.substr(1, zmq_msg_size(&message) - 2))) {
			subUUID = subChannel.substr(1, zmq_msg_size(&message) - 2);
		}

		if (subscription) {
			UM_LOG_INFO("%s: Got 0MQ subscription on %s", SHORT_UUID(_uuid).c_str(), subChannel.c_str());
			if (subUUID.length() > 0) {
				// every subscriber subscribes to its uuid prefixed with a "~" for late alphabetical order
				_subscriptions[subUUID].isZMQConfirmed = true;
				if (_subscriptions[subUUID].subStub)
					confirmSubscription(subUUID);
			}
		} else {
			UM_LOG_INFO("%s: Got 0MQ unsubscription on %s", SHORT_UUID(_uuid).c_str(), subChannel.c_str());
			if (subUUID.size() && _subscriptions[subUUID].isZMQConfirmed) {
				std::map<std::string, Publisher>::iterator pubIter = _subscriptions[subUUID].confirmed.begin();
				while(pubIter != _subscriptions[subUUID].confirmed.end()) {
					if(_connFrom.find(_subscriptions[subUUID].nodeUUID)!=_connFrom.end())
						pubIter->second.removed(_subscriptions[subUUID].subStub, _connFrom[_subscriptions[subUUID].nodeUUID]);
					else if(_connTo.find(_subscriptions[subUUID].nodeUUID)!=_connTo.end())
						pubIter->second.removed(_subscriptions[subUUID].subStub, _connTo[_subscriptions[subUUID].nodeUUID]->node);
					pubIter++;
				}
			}
		}

		zmq_getsockopt (_pubSocket, ZMQ_RCVMORE, &more, &moreSize) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
		zmq_msg_close (&message) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		assert(!more); // subscriptions are not multipart
		if (!more)
			break;      //  Last message part
	}

}

/**
 * Process messages sent to one of the client sockets from a remote node
 *
 * UM_PUB_REMOVED
 * UM_PUB_ADDED
 * UM_SHUTDOWN
 * UM_CONNECT_REP
 *
 */
void ZeroMQNode::receivedFromClientNode(SharedPtr<NodeConnection> client) {
	UM_TRACE("receivedFromClientNode");
	COMMON_VARS;

	// we have a reply from the server
	RECV_MSG(client->socket, opMsg);

	if (REMAINING_BYTES_TOREAD < 4) {
		zmq_msg_close(&opMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		return;
	}

	uint16_t type;
	uint16_t version;
	readPtr = readVersionAndType(recvBuffer, version, type);

	if (version != Message::UM_VERSION) {
		UM_LOG_INFO("%s: client socket received unversioned or different message format version - discarding", SHORT_UUID(_uuid).c_str());
		zmq_msg_close(&opMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		return;
	}

	UM_LOG_INFO("%s: client socket received %s from %s", SHORT_UUID(_uuid).c_str(), Message::typeToString(type), client->address.c_str());

	switch (type) {
	case Message::UM_PUB_REMOVED:
	case Message::UM_PUB_ADDED: {

		if (REMAINING_BYTES_TOREAD < 37) {
			break;
		}

		std::string from;
		readPtr = Message::read(readPtr, from, 37);

		if (_connTo.find(from) == _connTo.end()) {
			UM_LOG_WARN("%s: received publisher added/removed from %s, but we are not connected", SHORT_UUID(_uuid).c_str(), SHORT_UUID(from).c_str());
			break;
		}

		SharedPtr<PublisherStubImpl> pubStub = SharedPtr<PublisherStubImpl>(new PublisherStubImpl());
		readPtr = read(readPtr, pubStub.get(), REMAINING_BYTES_TOREAD);
		assert(REMAINING_BYTES_TOREAD == 0);

		if (type == Message::UM_PUB_ADDED) {
			receivedRemotePubAdded(_connTo[from], pubStub);
		} else {
			receivedRemotePubRemoved(_connTo[from], pubStub);
		}

	}
	case Message::UM_SHUTDOWN: {
		// a remote neighbor shut down
		if (REMAINING_BYTES_TOREAD < 37) {
			break;
		}

		std::string from;
		readPtr = Message::read(readPtr, from, 37);
		assert(REMAINING_BYTES_TOREAD == 0);

		if (_connFrom.find(from) == _connFrom.end()) {
			// node terminated, it's no longer connected to us
			_connFrom.erase(from);
		}

		// if we were connected, remove it, object will be destructed there
		if (_connTo.find(from) != _connTo.end()) {
			removed(_connTo[from]->node);
		}

		break;
	}
	case Message::UM_CONNECT_REP: {

		// remote server answered our connect_req
		if (REMAINING_BYTES_TOREAD < 37) {
			break;
		}

		std::string uuid;
		readPtr = Message::read(readPtr, uuid, 37);

		std::list<SharedPtr<PublisherStubImpl> > publishers;
		while(REMAINING_BYTES_TOREAD > 37) {
			SharedPtr<PublisherStubImpl> pubStub = SharedPtr<PublisherStubImpl>(new PublisherStubImpl());
			readPtr = read(readPtr, pubStub.get(), REMAINING_BYTES_TOREAD);
			publishers.push_back(pubStub);
		}

		remoteNodeConfirm(uuid, client, publishers);
		break;
	}
	default:
		UM_LOG_WARN("%s: Unhandled message type on client socket", SHORT_UUID(_uuid).c_str());
		break;
	}
	zmq_msg_close(&opMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

/**
 * Process messages sent to the internal operation socket.
 *
 * We need to dispatch / serialize all operations on the sockets into the zmq thread.
 *
 * UM_PUB_REMOVED
 * UM_PUB_ADDED
 * UM_DISCONNECT
 * UM_CONNECT_REQ
 * UM_SHUTDOWN
 */
void ZeroMQNode::receivedInternalOp() {
	UM_TRACE("receivedInternalOp");
	COMMON_VARS;

	// read first message
	RECV_MSG(_readOpSocket, opMsg)

	uint16_t type;
	uint16_t version;
	readPtr = readVersionAndType(recvBuffer, version, type);

//	UM_LOG_INFO("%s: internal op socket received %s", SHORT_UUID(_uuid).c_str(), Message::typeToString(type));
	switch (type) {
	case Message::UM_PUB_REMOVED:
	case Message::UM_PUB_ADDED: {
		// removePublisher / addPublisher called us with serialized publisher info

		std::string uuid;
		readPtr = Message::read(readPtr, uuid, 37);

		PublisherStubImpl* pubStub = new PublisherStubImpl();
		readPtr = read(readPtr, pubStub, REMAINING_BYTES_TOREAD);
		assert(REMAINING_BYTES_TOREAD == 0);

		std::string internalPubId("inproc://um.pub.intern.");
		internalPubId += pubStub->getUUID();

		delete pubStub;

		if (type == Message::UM_PUB_ADDED) {
			zmq_connect(_subSocket, internalPubId.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		} else {
			zmq_disconnect(_subSocket, internalPubId.c_str()) && UM_LOG_ERR("zmq_disconnect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		}
		// tell every other node connected to us that we changed publishers
		NODE_BROADCAST_MSG(opMsg);

		break;
	}

	case ZeroMQNode::UM_DISCONNECT: {
		// endpoint was removed
		std::string address;
		readPtr = Message::read(readPtr, address, REMAINING_BYTES_TOREAD);

		remoteNodeDisconnect(address);
		break;
	}
	case Message::UM_CONNECT_REQ: {
		// added(EndPoint) called us - rest of message is endpoint address and optional uuid
		std::string address;
		readPtr = Message::read(readPtr, address, REMAINING_BYTES_TOREAD);

		remoteNodeConnect(address);
		break;
	}
	case Message::UM_SHUTDOWN: {
		// do we need to do something here - destructor does most of the work
		break;
	}
	default:
		UM_LOG_WARN("%s: Unhandled message type on internal op socket", SHORT_UUID(_uuid).c_str());
		break;
	}
	zmq_msg_close(&opMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

/**
 * Prepare connection request to remote node
 */
void ZeroMQNode::remoteNodeConnect(const std::string& address) {
	COMMON_VARS

	assert(_connTo.find(address) == _connTo.end());
	SharedPtr<NodeConnection> clientConn = SharedPtr<NodeConnection>(new NodeConnection(address, _uuid));

	clientConn->address = address;
	int err = clientConn->connect();
	if (err)
		return;

	_connTo[address] = clientConn;
	_dirtySockets = true;

	UM_LOG_INFO("%s: Sending CONNECT_REQ to %s", SHORT_UUID(_uuid).c_str(), address.c_str());

	// send a CONNECT_REQ message
	PREPARE_MSG(connReqMsg, 4);
	writePtr = writeVersionAndType(writePtr, Message::UM_CONNECT_REQ);
	ASSERT_BYTES_WRITTEN(4);

	zmq_sendmsg(clientConn->socket, &connReqMsg, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
	zmq_msg_close(&connReqMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += 4;
	}
}

/**
 * Process connection reply message
 */
void ZeroMQNode::remoteNodeConfirm(const std::string& uuid, SharedPtr<NodeConnection> otherNode, const std::list<SharedPtr<PublisherStubImpl> >& publishers) {
	assert(otherNode->address.length() > 0);

	/**
	 * The socket of the other node replied with its uuid and publishers. We have
	 * to check whether our information is still current
	 */

	if (otherNode->node && otherNode->node.getUUID() != uuid) {
		UM_LOG_WARN("%s: Expecting connection reply from %s, but was answered by %s - detaching and reconnecting",
		            SHORT_UUID(_uuid).c_str(),
		            SHORT_UUID(otherNode->node.getUUID()).c_str(),
		            SHORT_UUID(uuid).c_str());

		// UUIDs were changed, this node is new or replaced an old one
		if (otherNode->isConfirmed) {
			// unsubscribe form old nodes publishers
			unsubscribeFromRemoteNode(otherNode);
		}

		if (otherNode->node.getUUID().length() > 0) {
			// delete records of old node
			_connTo.erase(otherNode->node.getUUID());
			_connFrom.erase(otherNode->node.getUUID());
		}

		// unset nodestub information
		otherNode->node = NodeStub();
	}

	if (!otherNode->node) {
		// needs a new or updated node

		size_t colonPos = otherNode->address.find_last_of(":");
		std::string transport = otherNode->address.substr(0,3);
		std::string ip = otherNode->address.substr(6, colonPos - 6);
		uint16_t port = strTo<uint16_t>(otherNode->address.substr(colonPos + 1, otherNode->address.length() - colonPos + 1));

		if (_connFrom.find(uuid) != _connFrom.end()) {
			otherNode->node = _connFrom[uuid];
		} else {
			otherNode->node = NodeStub(SharedPtr<NodeStubImpl>(new NodeStubImpl()));
		}
		otherNode->node.getImpl()->setUUID(uuid);
		otherNode->node.getImpl()->setTransport(transport);
		otherNode->node.getImpl()->setIP(ip);
		otherNode->node.getImpl()->setPort(port);
		otherNode->node.getImpl()->updateLastSeen();

		_connTo[uuid] = otherNode;

		std::list<SharedPtr<PublisherStubImpl> >::const_iterator pubIter = publishers.begin();
		while(pubIter != publishers.end()) {
			receivedRemotePubAdded(otherNode, *pubIter);
			pubIter++;
		}
	}

	otherNode->isConfirmed = true;
}

void ZeroMQNode::remoteNodeDisconnect(const std::string& address) {
	if (_connTo.find(address) == _connTo.end()) {
		UM_LOG_ERR("Received internal disconnect request for %s, but adress is no known", address.c_str());
		return;
	}

	SharedPtr<NodeConnection> otherNode = _connTo[address];
	if (otherNode->isConfirmed) {
		unsubscribeFromRemoteNode(otherNode);
		_connTo.erase(otherNode->node.getUUID());
		_connFrom.erase(otherNode->node.getUUID());
	}

	otherNode->disconnect();
	_connTo.erase(address);

	_dirtySockets = true;

}

void ZeroMQNode::updateSockets() {
	RScopeLock lock(_mutex);
	UM_TRACE("updateSockets");

	if (_sockets != NULL)
		free(_sockets);
	_nodeSockets.clear();

	size_t maxSockets = _connTo.size() + _nrStdSockets;
	_sockets = (zmq_pollitem_t*)malloc(sizeof(zmq_pollitem_t) * maxSockets);

	/**
	 * prepopulate with standard sockets:
	 *  _stdSockets[0].socket = _nodeSocket;
	 *  _stdSockets[1].socket = _pubSocket;
	 *  _stdSockets[2].socket = _readOpSocket;
	 *  _stdSockets[3].socket = _subSocket;
	 *
	 */
	memcpy(_sockets, _stdSockets, _nrStdSockets * sizeof(zmq_pollitem_t));

	size_t index = _nrStdSockets;
	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator sockIter = _connTo.begin();
	while (_connTo.size() > 0 && sockIter != _connTo.end()) {
		if (!UUID::isUUID(sockIter->first)) { // only add if key is an address
			_sockets[index].socket = sockIter->second->socket;
			_sockets[index].fd = 0;
			_sockets[index].events = ZMQ_POLLIN;
			_sockets[index].revents = 0;
			_nodeSockets.push_back(std::make_pair(index, sockIter->first));
			index++;
		}
		sockIter++;
	}
	_nrSockets = index;
	_dirtySockets = false;
}

void ZeroMQNode::run() {
	UM_TRACE("run");
	int more;
	size_t moreSize = sizeof(more);

	while(isStarted()) {
		// make sure we have a bucket for performance measuring
		if (_buckets.size() == 0)
			_buckets.push_back(StatBucket<size_t>());

		if (_dirtySockets)
			updateSockets();

		for (int i = 0; i < _nrSockets; i++) {
			_sockets[i].revents = 0;
		}

		zmq_poll(_sockets, _nrSockets, -1);
		// We do have a message to read!

		// manage performane status buckets
		uint64_t now = Thread::getTimeStampMs();
		while (_buckets.size() > 0 && _buckets.front().timeStamp < now - UMUNDO_PERF_WINDOW_LENGTH_MS) {
			// drop oldest bucket
			_buckets.pop_front();
		}
		if (_buckets.back().timeStamp < now - UMUNDO_PERF_BUCKET_LENGTH_MS) {
			// we need a new bucket
			_buckets.push_back(StatBucket<size_t>());
		}

		// look through node sockets
		std::list<std::pair<uint32_t, std::string> >::const_iterator nodeSockIter = _nodeSockets.begin();
		while(nodeSockIter != _nodeSockets.end()) {
			if (_sockets[nodeSockIter->first].revents & ZMQ_POLLIN) {
				RScopeLock lock(_mutex);
				if (_connTo.find(nodeSockIter->second) != _connTo.end()) {
					receivedFromClientNode(_connTo[nodeSockIter->second]);
				} else {
					UM_LOG_WARN("%s: message from vanished node %s", _uuid.c_str(), nodeSockIter->second.c_str());
				}
				break;
			}
			nodeSockIter++;
		}


		if (_sockets[0].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			receivedFromNodeSocket();
			DRAIN_SOCKET(_nodeSocket);
		}

		if (_sockets[1].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			receivedFromPubSocket();
			DRAIN_SOCKET(_pubSocket);
		}

		if (_sockets[2].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			receivedInternalOp();
			DRAIN_SOCKET(_readOpSocket);
		}

		// someone is publishing via our external publisher, just pass through
		if (_sockets[3].revents & ZMQ_POLLIN) {
			size_t msgSize = 0;
			zmq_msg_t message;
			std::string channelName;
			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno));
				zmq_msg_recv (&message, _subSocket, 0);
				msgSize = zmq_msg_size(&message);

				if (channelName.size() == 0)
					channelName = std::string((char*)zmq_msg_data(&message));

				if (_buckets.size() > 0) {
					_buckets.back().nrChannelMsg[channelName]++;
					_buckets.back().sizeChannelMsg[channelName] += msgSize;
				}
				zmq_getsockopt (_subSocket, ZMQ_RCVMORE, &more, &moreSize) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
				zmq_msg_send(&message, _pubSocket, more ? ZMQ_SNDMORE: 0) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
				zmq_msg_close (&message) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
				if (!more)
					break;      //  Last message part
			}
		}

//			if (now - _lastNodeInfoBroadCast > 5000) {
//				broadCastNodeInfo(now);
//				_lastNodeInfoBroadCast = now;
//			}
//			if (now - _lastDeadNodeRemoval > 15000) {
//				removeStaleNodes(now);
//				_lastDeadNodeRemoval = now;
//			}
	}
}

#if 0
void ZeroMQNode::broadCastNodeInfo(uint64_t now) {
	UM_TRACE("broadCastNodeInfo");
	zmq_msg_t infoMsg;
	writeNodeInfo(&infoMsg, Message::UM_NODE_INFO);
	NODE_BROADCAST_MSG(infoMsg);
	zmq_msg_close(&infoMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

void ZeroMQNode::removeStaleNodes(uint64_t now) {
	RScopeLock lock(_mutex);
	UM_TRACE("removeStaleNodes");
	std::map<std::string, SharedPtr<NodeConnection> >::iterator pendingNodeIter = _connTo.begin();
	while(_connTo.size() > 0 && pendingNodeIter != _connTo.end()) {
		if (now - pendingNodeIter->second->startedAt > 30000) {
			// we have been very patient remove pending node
			UM_LOG_ERR("%s could not connect to node at %s - removing", SHORT_UUID(_uuid).c_str(), pendingNodeIter->first.c_str());
//			delete pendingNodeIter->second;
			_connTo.erase(pendingNodeIter++);
			_dirtySockets = true;
		} else {
			pendingNodeIter++;
		}
	}

	std::map<std::string, NodeStub>::iterator connIter = _connFrom.begin();
	while(connIter != _connFrom.end()) {
		if (!UUID::isUUID(connIter->first)) {
			connIter++;
			continue;
		}
		if (now - connIter->second.getLastSeen() > 30000) {
			// we have been very patient remove pending node
			UM_LOG_ERR("%s timeout for %s - removing", SHORT_UUID(_uuid).c_str(), SHORT_UUID(connIter->second.getUUID()).c_str());
			NodeStub nodeStub = connIter->second;
			// Disconnect subscribers
			std::map<std::string, PublisherStub> pubStubs = nodeStub.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubStubIter = pubStubs.begin();
			while(pubStubIter != pubStubs.end()) {
				std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
				while(subIter != _subs.end()) {
					if (subIter->second.matches(pubStubIter->second))
						subIter->second.removed(pubStubIter->second, nodeStub);
					subIter++;
				}
				pubStubIter++;
			}

			if (connIter->second->address.length())
				_connFrom.erase(connIter->second->address);
//			delete connIter->second;
			_connFrom.erase(connIter++);
		} else {
			connIter++;
		}
	}
}
#endif

/**
 * Disconnect local publishers and subscribers and send unsubscribe messages
 */
void ZeroMQNode::unsubscribeFromRemoteNode(SharedPtr<NodeConnection> connection) {
	UM_TRACE("unsubscribeFromRemoteNode");
	if (connection->node) {
		std::string nodeUUID = connection->node.getUUID();
		std::map<std::string, PublisherStub> remotePubs = connection->node.getPublishers();
		std::map<std::string, PublisherStub>::iterator remotePubIter = remotePubs.begin();
		std::map<std::string, Subscriber>::iterator localSubIter = _subs.begin();

		// iterate all remote publishers and remove from local subs
		while (remotePubIter != remotePubs.end()) {
			while (localSubIter != _subs.end()) {
				if(localSubIter->second.matches(remotePubIter->second)) {
					localSubIter->second.removed(remotePubIter->second, connection->node);
					sendUnsubscribeFromPublisher(connection->node.getUUID().c_str(), localSubIter->second, remotePubIter->second);
				}
				localSubIter++;
			}

			std::list<ResultSet<PublisherStub>* >::iterator monitorIter = _pubMonitors.begin();
			while(monitorIter != _pubMonitors.end()) {
				(*monitorIter)->remove(remotePubIter->second, toStr(this));
				(*monitorIter)->change(remotePubIter->second, toStr(this));
				monitorIter++;
			}

			remotePubIter++;
		}

		std::map<std::string, SubscriberStub> remoteSubs = connection->node.getSubscribers();
		std::map<std::string, SubscriberStub>::iterator remoteSubIter = remoteSubs.begin();
		while(remoteSubIter != remoteSubs.end()) {
			if (_subscriptions.find(remoteSubIter->first) != _subscriptions.end()) {
				Subscription& subscription = _subscriptions[remoteSubIter->first];
				std::map<std::string, Publisher>::iterator confirmedIter = subscription.confirmed.begin();
				while(confirmedIter != subscription.confirmed.end()) {
					confirmedIter->second.removed(subscription.subStub, connection->node);
					confirmedIter++;
				}
				_subscriptions.erase(remoteSubIter->first);
			} else {
				// could not find any subscriptions
			}
			remoteSubIter++;
		}
	}
}

void ZeroMQNode::confirmSubscription(const std::string& subUUID) {
	UM_TRACE("confirmSubscription");
	if (_subscriptions.find(subUUID) == _subscriptions.end())
		return;

	Subscription& pendSub = _subscriptions[subUUID];

	if (!pendSub.subStub)
		return;

	if (pendSub.subStub.getImpl()->implType == Subscriber::ZEROMQ &&
	        !pendSub.isZMQConfirmed)
		return;

	if (_connFrom.find(pendSub.nodeUUID) != _connFrom.end()) {
		_connFrom[pendSub.nodeUUID].addSubscriber(pendSub.subStub);
	}

	// copy over address from getpeer*
	if (_connFrom[pendSub.nodeUUID].getIP().length() == 0 && pendSub.address.length() > 0)
		_connFrom[pendSub.nodeUUID].getImpl()->setIP(pendSub.address);

	// move all pending subscriptions to confirmed
	std::map<std::string, Publisher>::iterator pendPubIter = pendSub.pending.begin();
	while(pendPubIter != pendSub.pending.end()) {
		pendPubIter->second.added(pendSub.subStub, _connFrom[pendSub.nodeUUID]);
		pendSub.confirmed[pendPubIter->first] = pendPubIter->second;
		pendPubIter++;
	}
	pendSub.pending.clear();
}

void ZeroMQNode::writeNodeInfo(zmq_msg_t* msg, Message::Type type) {
	UM_TRACE("writeNodeInfo");

	zmq_msg_init(msg) && UM_LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno));

	size_t pubInfoSize = 0;
	std::map<std::string, Publisher>::iterator pubIter =_pubs.begin();
	while(pubIter != _pubs.end()) {
		pubInfoSize += PUB_INFO_SIZE(pubIter->second);
		pubIter++;
	}

	zmq_msg_init_size (msg, 4 + _uuid.length() + 1 + pubInfoSize) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno));
	char* writeBuffer = (char*)zmq_msg_data(msg);
	char* writePtr = writeBuffer;

	// version and type
	writePtr = writeVersionAndType(writePtr, type);
	assert(writePtr - writeBuffer == 4);

	// local uuid
	writePtr = Message::write(writePtr, _uuid);
	assert(writePtr - writeBuffer == 4 + _uuid.length() + 1);

	pubIter =_pubs.begin();
	while(pubIter != _pubs.end()) {
		writePtr = write(writePtr, pubIter->second);
		pubIter++;
	}

	assert(writePtr - writeBuffer == zmq_msg_size(msg));
}

void ZeroMQNode::receivedRemotePubAdded(SharedPtr<NodeConnection> client, SharedPtr<PublisherStubImpl> pub) {
	UM_TRACE("receivedRemotePubAdded");

	NodeStub nodeStub = client->node;
	nodeStub.updateLastSeen();

	PublisherStub pubStub(pub);
	pubStub.getImpl()->setDomain(nodeStub.getUUID());

	pubStub.getImpl()->setInProcess(nodeStub.isInProcess());
	pubStub.getImpl()->setRemote(nodeStub.isRemote());
	pubStub.getImpl()->setIP(nodeStub.getIP());
	pubStub.getImpl()->setTransport(nodeStub.getTransport());

	nodeStub.getImpl()->addPublisher(pubStub);

	std::list<ResultSet<PublisherStub>* >::iterator monitorIter = _pubMonitors.begin();
	while(monitorIter != _pubMonitors.end()) {
		(*monitorIter)->add(pubStub, toStr(this));
		(*monitorIter)->change(pubStub, toStr(this));
		monitorIter++;
	}

	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		if (subIter->second.matches(pubStub)) {
			subIter->second.added(pubStub, nodeStub);
			sendSubscribeToPublisher(nodeStub.getUUID(), subIter->second, pubStub);
		}
		subIter++;
	}
}

void ZeroMQNode::receivedRemotePubRemoved(SharedPtr<NodeConnection> client, SharedPtr<PublisherStubImpl> pub) {
	UM_TRACE("receivedRemotePubRemoved");

	NodeStub nodeStub = client->node;
	PublisherStub pubStub = nodeStub.getPublisher(pub->getUUID());

	if (!pubStub) {
		return;
	}

	std::list<ResultSet<PublisherStub>* >::iterator monitorIter = _pubMonitors.begin();
	while(monitorIter != _pubMonitors.end()) {
		(*monitorIter)->remove(pubStub, toStr(this));
		(*monitorIter)->change(pubStub, toStr(this));
		monitorIter++;
	}

	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		if (subIter->second.matches(pubStub)) {
			subIter->second.removed(pubStub, nodeStub);
			sendUnsubscribeFromPublisher(nodeStub.getUUID(), subIter->second, pubStub);
		}
		subIter++;
	}
	nodeStub.removePublisher(pubStub);
}

void ZeroMQNode::sendSubscribeToPublisher(const std::string& nodeUUID, const Subscriber& sub, const PublisherStub& pub) {
	UM_TRACE("sendSubscribeToPublisher");
	COMMON_VARS;

	if (_connTo.find(nodeUUID) == _connTo.end()) {
		UM_LOG_WARN("Not sending sub added for %s on %s to publisher %s - node unknown",
		            sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());
		return;
	}

	void* clientSocket = _connTo[nodeUUID]->socket;
	if (!clientSocket) {
		UM_LOG_WARN("Not sending sub added for %s on %s to publisher %s - node socket not connected",
		            sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());
		return;
	}

	UM_LOG_INFO("Sending sub added for %s on %s to publisher %s",
	            sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());

	size_t bufferSize = 4 + SUB_INFO_SIZE(sub) + PUB_INFO_SIZE(pub);
	PREPARE_MSG(subAddedMsg, bufferSize);

	writePtr = writeVersionAndType(writePtr, Message::UM_SUBSCRIBE);
	writePtr = write(writePtr, sub);
	writePtr = write(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&subAddedMsg, clientSocket, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}
	zmq_msg_close(&subAddedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

void ZeroMQNode::sendUnsubscribeFromPublisher(const std::string& nodeUUID, const Subscriber& sub, const PublisherStub& pub) {
	UM_TRACE("sendUnsubscribeFromPublisher");
	COMMON_VARS;

	if (_connTo.find(nodeUUID) == _connTo.end())
		return;

	void* clientSocket = _connTo[nodeUUID]->socket;
	if (!clientSocket)
		return;

	UM_LOG_INFO("Sending sub removed for %s on %s to publisher %s",
	            sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());

	size_t bufferSize = 4 + SUB_INFO_SIZE(sub) + PUB_INFO_SIZE(pub);
	PREPARE_MSG(subRemovedMsg, bufferSize);

	writePtr = writeVersionAndType(writePtr, Message::UM_UNSUBSCRIBE);
	writePtr = write(writePtr, sub);
	writePtr = write(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&subRemovedMsg, clientSocket, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}
	zmq_msg_close(&subRemovedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

}

char* ZeroMQNode::write(char* buffer, const PublisherStub& pub) {
	char* start = buffer;
	(void)start; // surpress unused warning wiithout assert

	buffer = Message::write(buffer, pub.getChannelName());
	buffer = Message::write(buffer, pub.getUUID());
	buffer = Message::write(buffer, pub.getImpl()->implType);
	buffer = Message::write(buffer, (pub.getImpl()->implType == ZMQ_PUB ? _pubPort : pub.getPort()));

	assert(buffer - start == PUB_INFO_SIZE(pub));
	return buffer;
}

const char* ZeroMQNode::read(const char* buffer, PublisherStubImpl* pubStub, size_t available) {
	const char* start = buffer;
	(void)start; // surpress unused warning without assert

	std::string channelName;
	buffer = Message::read(buffer, channelName, available - (buffer - start));
	pubStub->setChannelName(channelName);

	std::string uuid;
	buffer = Message::read(buffer, uuid, available - (buffer - start));
	pubStub->setUUID(uuid);

	uint16_t type;
	buffer = Message::read(buffer, &type);
	pubStub->implType = type;

	uint16_t port;
	buffer = Message::read(buffer, &port);
	pubStub->setPort(port);

	return buffer;
}

char* ZeroMQNode::write(char* buffer, const Subscriber& sub) {
	assert(sub.getUUID().length() == 36);

	char* start = buffer;
	(void)start; // surpress unused warning without assert

	buffer = Message::write(buffer, sub.getChannelName());
	buffer = Message::write(buffer, sub.getUUID());
	buffer = Message::write(buffer, (uint16_t)sub.getImpl()->implType);
	buffer = Message::write(buffer, (uint16_t)(sub.isMulticast() ? 1 : 0));
	buffer = Message::write(buffer, sub.getIP());
	buffer = Message::write(buffer, sub.getPort());

	assert(buffer - start == SUB_INFO_SIZE(sub));
	return buffer;
}

const char* ZeroMQNode::read(const char* buffer, SubscriberStubImpl* subStub, size_t available) {
	const char* start = buffer;
	(void)start; // surpress unused warning without assert

	std::string channelName;
	buffer = Message::read(buffer, channelName, available - (buffer - start));
	subStub->setChannelName(channelName);

	std::string uuid;
	buffer = Message::read(buffer, uuid, available - (buffer - start));
	subStub->setUUID(uuid);

	uint16_t type;
	buffer = Message::read(buffer, &type);
	subStub->implType = type;

	uint16_t multicast;
	buffer = Message::read(buffer, &multicast);
	subStub->setMulticast(multicast == 1 ? true : false);

	std::string ip;
	buffer = Message::read(buffer, ip, available - (buffer - start));
	subStub->setIP(ip);

	uint16_t port;
	buffer = Message::read(buffer, &port);
	subStub->setPort(port);

	return buffer;
}

char* ZeroMQNode::writeVersionAndType(char* buffer, uint16_t type) {
	buffer = Message::write(buffer, (uint16_t)Message::UM_VERSION);
	buffer = Message::write(buffer, (uint16_t)type);
	return buffer;
}

const char* ZeroMQNode::readVersionAndType(const char* buffer, uint16_t& version, uint16_t& type) {
	// clear out of type bytes
//	memset(&type, 0, sizeof(type));

	buffer = Message::read(buffer, &version);
	buffer = Message::read(buffer, &(uint16_t&)type);
	return buffer;
}


ZeroMQNode::StatBucket<double> ZeroMQNode::accumulateIntoBucket() {
	StatBucket<double> statBucket;

	double rollOffFactor = 0.3;

	std::list<StatBucket<size_t> >::iterator buckFrameStart = _buckets.begin();
	std::list<StatBucket<size_t> >::iterator buckFrameEnd = _buckets.begin();
	std::map<std::string, size_t>::iterator chanIter;

	while(buckFrameEnd != _buckets.end()) {
		if (buckFrameEnd->timeStamp - 1000 < buckFrameStart->timeStamp) {
			// we do not yet have a full second
			buckFrameEnd++;
			continue;
		}

		// accumulate stats for a second
		StatBucket<size_t> oneSecBucket;
		std::list<StatBucket<size_t> >::iterator curr = buckFrameStart;
		while(curr != buckFrameEnd) {
			oneSecBucket.nrMetaMsgRcvd += curr->nrMetaMsgRcvd;
			oneSecBucket.nrMetaMsgSent += curr->nrMetaMsgSent;
			oneSecBucket.sizeMetaMsgRcvd += curr->sizeMetaMsgRcvd;
			oneSecBucket.sizeMetaMsgSent += curr->sizeMetaMsgSent;

			chanIter = curr->nrChannelMsg.begin();
			while (chanIter != curr->nrChannelMsg.end()) {
				oneSecBucket.nrChannelMsg[chanIter->first] += chanIter->second;
				chanIter++;
			}
			chanIter = curr->sizeChannelMsg.begin();
			while (chanIter != curr->sizeChannelMsg.end()) {
				oneSecBucket.sizeChannelMsg[chanIter->first] += chanIter->second;
				chanIter++;
			}
			curr++;
		}

		statBucket.nrMetaMsgSent = (1 - rollOffFactor) * statBucket.nrMetaMsgSent + rollOffFactor * oneSecBucket.nrMetaMsgSent;
		statBucket.nrMetaMsgRcvd = (1 - rollOffFactor) * statBucket.nrMetaMsgRcvd + rollOffFactor * oneSecBucket.nrMetaMsgRcvd;
		statBucket.sizeMetaMsgSent = (1 - rollOffFactor) * statBucket.sizeMetaMsgSent + rollOffFactor * oneSecBucket.sizeMetaMsgSent;
		statBucket.sizeMetaMsgRcvd = (1 - rollOffFactor) * statBucket.sizeMetaMsgRcvd + rollOffFactor * oneSecBucket.sizeMetaMsgRcvd;

		chanIter = oneSecBucket.sizeChannelMsg.begin();
		while (chanIter != oneSecBucket.sizeChannelMsg.end()) {
			statBucket.sizeChannelMsg[chanIter->first] = (1 - rollOffFactor) * statBucket.sizeChannelMsg[chanIter->first] + rollOffFactor * chanIter->second;;
			chanIter++;
		}

		chanIter = oneSecBucket.nrChannelMsg.begin();
		while (chanIter != oneSecBucket.nrChannelMsg.end()) {
			statBucket.nrChannelMsg[chanIter->first] = (1 - rollOffFactor) * statBucket.nrChannelMsg[chanIter->first] + rollOffFactor * chanIter->second;;
			chanIter++;
		}

		buckFrameStart++;
	}

	return statBucket;
}

void ZeroMQNode::replyWithDebugInfo(const std::string uuid) {
	StatBucket<double> statBucket = accumulateIntoBucket();

	// return to sender
	zmq_send(_nodeSocket, uuid.c_str(), uuid.length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	zmq_send(_nodeSocket, "", 0, ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));

	std::stringstream ss;

	// first identify us
	SEND_DEBUG_ENVELOPE(std::string("uuid:" + _uuid));
	SEND_DEBUG_ENVELOPE(std::string("host:" + hostUUID));

#ifdef APPLE
	SEND_DEBUG_ENVELOPE(std::string("os: MacOSX"));
#elif defined CYGWIN
	SEND_DEBUG_ENVELOPE(std::string("os: Cygwin"));
#elif defined ANDROID
	SEND_DEBUG_ENVELOPE(std::string("os: Android"));
#elif defined IOS
	SEND_DEBUG_ENVELOPE(std::string("os: iOS"));
#elif defined UNIX
	SEND_DEBUG_ENVELOPE(std::string("os: Unix"));
#elif defined WIN32
	SEND_DEBUG_ENVELOPE(std::string("os: Windows"));
#endif

	SEND_DEBUG_ENVELOPE(std::string("proc:" + procUUID));

	// calculate performance
	SEND_DEBUG_ENVELOPE(std::string("sent:msgs:" + toStr(ceil(statBucket.nrMetaMsgSent))));
	SEND_DEBUG_ENVELOPE(std::string("sent:bytes:" + toStr(ceil(statBucket.sizeMetaMsgSent))));
	SEND_DEBUG_ENVELOPE(std::string("rcvd:msgs:" + toStr(ceil(statBucket.nrMetaMsgRcvd))));
	SEND_DEBUG_ENVELOPE(std::string("rcvd:bytes:" + toStr(ceil(statBucket.sizeMetaMsgRcvd))));

	// send our publishers
	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while (pubIter != _pubs.end()) {

		SEND_DEBUG_ENVELOPE(std::string("pub:uuid:" + pubIter->first));
		SEND_DEBUG_ENVELOPE(std::string("pub:channelName:" + pubIter->second.getChannelName()));
		SEND_DEBUG_ENVELOPE(std::string("pub:type:" + toStr(pubIter->second.getImpl()->implType)));
		SEND_DEBUG_ENVELOPE(std::string("pub:sent:msgs:" + toStr(ceil(statBucket.nrChannelMsg[pubIter->second.getChannelName()]))));
		SEND_DEBUG_ENVELOPE(std::string("pub:sent:bytes:" + toStr(ceil(statBucket.sizeChannelMsg[pubIter->second.getChannelName()]))));

		std::map<std::string, SubscriberStub> subs = pubIter->second.getSubscribers();
		std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();

		// send its subscriptions as well
		while (subIter != subs.end()) {
			SEND_DEBUG_ENVELOPE(std::string("pub:sub:uuid:" + subIter->first));
			SEND_DEBUG_ENVELOPE(std::string("pub:sub:channelName:" + subIter->second.getChannelName()));
			SEND_DEBUG_ENVELOPE(std::string("pub:sub:type:" + toStr(subIter->second.getImpl()->implType)));

			subIter++;
		}
		pubIter++;
	}

	// send our subscribers
	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while (subIter != _subs.end()) {

		SEND_DEBUG_ENVELOPE(std::string("sub:uuid:" + subIter->first));
		SEND_DEBUG_ENVELOPE(std::string("sub:channelName:" + subIter->second.getChannelName()));
		SEND_DEBUG_ENVELOPE(std::string("sub:type:" + toStr(subIter->second.getImpl()->implType)));

		std::map<std::string, PublisherStub> pubs = subIter->second.getPublishers();
		std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
		// send all remote publishers we think this node has
		while (pubIter != pubs.end()) {
			SEND_DEBUG_ENVELOPE(std::string("sub:pub:uuid:" + pubIter->first));
			SEND_DEBUG_ENVELOPE(std::string("sub:pub:channelName:" + pubIter->second.getChannelName()));
			SEND_DEBUG_ENVELOPE(std::string("sub:pub:type:" + toStr(pubIter->second.getImpl()->implType)));

			pubIter++;
		}
		subIter++;
	}


	// send all the nodes we know about
	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator nodeIter;

	nodeIter = _connTo.begin();
	while (nodeIter != _connTo.end()) {
		// only report UUIDs as keys
		if (UUID::isUUID(nodeIter->first)) {
			SEND_DEBUG_ENVELOPE(std::string("conn:uuid:" + nodeIter->first));
			SEND_DEBUG_ENVELOPE(std::string("conn:address:" + nodeIter->second->address));
			SEND_DEBUG_ENVELOPE(std::string("conn:from:" + toStr(_connFrom.find(nodeIter->first) != _connFrom.end())));
			SEND_DEBUG_ENVELOPE(std::string("conn:to:" + toStr(true)));
			SEND_DEBUG_ENVELOPE(std::string("conn:confirmed:" + toStr(nodeIter->second->isConfirmed)));
			SEND_DEBUG_ENVELOPE(std::string("conn:startedAt:" + toStr(nodeIter->second->startedAt)));

			// send info about all nodes connected to us
			if (nodeIter->second->node) {
				NodeStub& nodeStub = nodeIter->second->node;

				SEND_DEBUG_ENVELOPE(std::string("conn:lastSeen:" + toStr(nodeStub.getLastSeen())));

				// send all remote publishers we think this node has
				std::map<std::string, PublisherStub> pubs = nodeStub.getPublishers();
				std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
				while (pubIter != pubs.end()) {
					SEND_DEBUG_ENVELOPE(std::string("conn:pub:uuid:" + pubIter->first));
					SEND_DEBUG_ENVELOPE(std::string("conn:pub:channelName:" + pubIter->second.getChannelName()));
					SEND_DEBUG_ENVELOPE(std::string("conn:pub:type:" + toStr(pubIter->second.getImpl()->implType)));

					pubIter++;
				}

				// send all remote subscribers we think this node has
				std::map<std::string, SubscriberStub> subs = nodeStub.getSubscribers();
				std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
				while (subIter != subs.end()) {
					SEND_DEBUG_ENVELOPE(std::string("conn:sub:uuid:" + subIter->first));
					SEND_DEBUG_ENVELOPE(std::string("conn:sub:channelName:" + subIter->second.getChannelName()));
					SEND_DEBUG_ENVELOPE(std::string("conn:sub:type:" + toStr(subIter->second.getImpl()->implType)));

					subIter++;
				}

			}
		}
		nodeIter++;
	}

	std::string stanzaDone = "done:" + _uuid;
	zmq_send(_nodeSocket, stanzaDone.c_str(), stanzaDone.length(), ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));

}

ZeroMQNode::NodeConnection::NodeConnection(const std::string& _socketId) :
	socket(NULL),
	socketId(_socketId),
	isConfirmed(false) {

	UM_TRACE("NodeConnection");
	startedAt	= Thread::getTimeStampMs();
}

ZeroMQNode::NodeConnection::NodeConnection(const std::string& _address, const std::string& _socketId) :
	socket(NULL),
	address(_address),
	socketId(_socketId),
	isConfirmed(false) {

	UM_TRACE("NodeConnection");
	startedAt	= Thread::getTimeStampMs();
}

int ZeroMQNode::NodeConnection::connect() {
	if (socket)
		return 0;

	int err = 0;
	socket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_DEALER);
	if (!socket) {
		UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
		return errno;
	}

	err = zmq_setsockopt(socket, ZMQ_IDENTITY, socketId.c_str(), socketId.length());
	if (err) {
		UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
		return err;
	}

	err = zmq_connect(socket, address.c_str());
	if (err) {
		UM_LOG_ERR("zmq_connect %s: %s", address.c_str(), zmq_strerror(errno));
		zmq_close(socket);
		socket = NULL;
	}
	return err;
}

int ZeroMQNode::NodeConnection::disconnect() {
	if (!socket)
		return 0;

	int err = 0;

	err = zmq_disconnect(socket, address.c_str());
	if (err) {
		UM_LOG_ERR("zmq_disconnect %s: %s", address.c_str(), zmq_strerror(errno));
		err = errno;
	} else {
		err = zmq_close(socket);
		if (err) {
			UM_LOG_ERR("zmq_disconnect %s: %s", address.c_str(), zmq_strerror(errno));
			err = errno;
		}
	}

	socket = NULL;
	return err;
}

ZeroMQNode::NodeConnection::~NodeConnection() {
	UM_TRACE("~NodeConnection");
	disconnect();
}

ZeroMQNode::Subscription::Subscription() :
	isZMQConfirmed(false),
	startedAt(Thread::getTimeStampMs()) {
	UM_TRACE("Subscription");
}

}
