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

#define DRAIN_SOCKET(socket) \
for(;;) { \
zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno)); \
if (!more) \
break; \
UM_LOG_INFO("Superfluous message on " #socket ""); \
zmq_recv(socket, NULL, 0, 0) && UM_LOG_ERR("zmq_recv: %s", zmq_strerror(errno)); \
}

#define MORE_OR_RETURN(socket, msg) \
zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));\
if (!more) { \
if (strlen(msg) > 0)\
UM_LOG_WARN(#msg " on " #socket ""); \
return; \
}

#define COMMON_VARS \
int more; (void)more;\
size_t more_size = sizeof(more); (void)more_size;\
int msgSize; (void)msgSize;\
char* recvBuffer; (void)recvBuffer;\
char* readPtr; (void)readPtr;\
char* writeBuffer; (void)writeBuffer;\
char* writePtr; (void)writePtr;

#define RECV_MSG(socket, msg) \
zmq_msg_t msg; \
zmq_msg_init(&msg) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_recvmsg(socket, &msg, 0) == -1 && UM_LOG_ERR("zmq_recvmsg: %s", zmq_strerror(errno)); \
msgSize = zmq_msg_size(&msg); \
recvBuffer = (char*)zmq_msg_data(&msg); \
readPtr = recvBuffer;

#define REMAINING_BYTES_TOREAD \
(msgSize - (readPtr - recvBuffer))

#define PUB_INFO_SIZE(pub) \
pub.getChannelName().length()+1 + pub.getUUID().length()+1 + sizeof(uint16_t) + sizeof(uint16_t)

#define SUB_INFO_SIZE(sub) \
sub.getChannelName().length()+1 + sub.getUUID().length()+1 + sizeof(uint16_t) + sizeof(uint16_t) + sub.getIP().length()+1 + sizeof(uint16_t)

#define PREPARE_MSG(msg, size) \
zmq_msg_t msg; \
zmq_msg_init(&msg) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno));\
writeBuffer = (char*)zmq_msg_data(&msg);\
writePtr = writeBuffer;\
 
#define NODE_BROADCAST_MSG(msg) \
std::map<std::string, SharedPtr<NodeConnection> >::iterator nodeIter_ = _connFrom.begin();\
while (nodeIter_ != _connFrom.end()) {\
	if (UUID::isUUID(nodeIter_->first)) {\
		zmq_msg_t broadCastMsgCopy_;\
		zmq_msg_init(&broadCastMsgCopy_) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno));\
		zmq_msg_copy(&broadCastMsgCopy_, &msg) && UM_LOG_ERR("zmq_msg_copy: %s", zmq_strerror(errno));\
		UM_LOG_DEBUG("%s: Broadcasting to %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(nodeIter_->first).c_str()); \
		zmq_send(_nodeSocket, nodeIter_->first.c_str(), nodeIter_->first.length(), ZMQ_SNDMORE | ZMQ_DONTWAIT); \
		zmq_msg_send(&broadCastMsgCopy_, _nodeSocket, ZMQ_DONTWAIT); \
		zmq_msg_close(&broadCastMsgCopy_) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));\
	}\
	nodeIter_++;\
}

#define RESETSS(ss) ss.clear(); ss.str(std::string(""));

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
	writePtr = writeString(writePtr, _uuid.c_str(), _uuid.length());
	assert(writePtr - writeBuffer == zmq_msg_size(&shutdownMsg));

	NODE_BROADCAST_MSG(shutdownMsg);
	zmq_msg_close(&shutdownMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

	// delete all connection information
	while(_connTo.size() > 0) {
		NodeStub node = _connTo.begin()->second->node;
		if(node) {
			removed(node);
			processOpComm();
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
	zmq_close(_subSocket)     && UM_LOG_ERR("zmq_close: %s",zmq_strerror(errno));
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

	// connect read and write op sockets
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

	std::map<std::string, NodeStub> from;
	std::map<std::string, SharedPtr<NodeConnection> >::iterator nodeIter = _connFrom.begin();
	while (nodeIter != _connFrom.end()) {
		// this will report ourself as well
		from[nodeIter->first] = nodeIter->second->node;
		nodeIter++;
	}
	return from;
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
		if (nodeIter->second && nodeIter->second->node) {
			std::map<std::string, PublisherStub> pubs = nodeIter->second->node.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();

			// iterate all remote publishers and add this sub
			while (pubIter != pubs.end()) {
				if(sub.matches(pubIter->second.getChannelName())) {
					sub.added(pubIter->second, nodeIter->second->node);
					sendSubAdded(nodeIter->first.c_str(), sub, pubIter->second);
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
				if(sub.matches(pubIter->second.getChannelName())) {
					sub.removed(pubIter->second, nodeIter->second->node);
					sendSubRemoved(nodeIter->first.c_str(), sub, pubIter->second);
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
	writePtr = writeString(writePtr, _uuid.c_str(), _uuid.length());
	writePtr = writePubInfo(writePtr, pub);
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
	writePtr = writeString(writePtr, _uuid.c_str(), _uuid.length());
	writePtr = writePubInfo(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&pubRemovedMsg, _writeOpSocket, 0) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}

	zmq_msg_close(&pubRemovedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
	_pubs.erase(pub.getUUID());

}

void ZeroMQNode::added(ENDPOINT_RS_TYPE endPoint) {
	RScopeLock lock(_mutex);
	UM_TRACE("added");

	if (_endPoints.find(endPoint) != _endPoints.end()) {
		UM_LOG_INFO("%s: Ignoring addition of endpoint at %s - already known",
								SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str());
		return;
	}
	_endPoints.insert(endPoint);

	UM_LOG_INFO("%s: Adding endpoint at %s",
	            SHORT_UUID(_uuid).c_str(),
	            endPoint.getAddress().c_str());

	COMMON_VARS;

	std::stringstream otherAddress;
	otherAddress << endPoint.getTransport() << "://" << ((endPoint.getIP().size() == 0 || endPoint.getIP() == "*") ? "localhost" : endPoint.getIP()) << ":" << endPoint.getPort();

	// write connection request to operation socket
	PREPARE_MSG(addEndPointMsg, 4 + otherAddress.str().length() + 1);

	writePtr = writeVersionAndType(writePtr, Message::UM_CONNECT_REQ);
	assert(writePtr - writeBuffer == 4);
	writePtr = writeString(writePtr, otherAddress.str().c_str(), otherAddress.str().length());
	assert(writePtr - writeBuffer == 4 + otherAddress.str().length() + 1);

	zmq_sendmsg(_writeOpSocket, &addEndPointMsg, 0) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
	zmq_msg_close(&addEndPointMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

void ZeroMQNode::removed(ENDPOINT_RS_TYPE endPoint) {
	RScopeLock lock(_mutex);
	UM_TRACE("removed");

	if (_endPoints.find(endPoint) == _endPoints.end()) {
		UM_LOG_INFO("%s: Ignoring removal of endpoint at %s - not known",
								SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str());
		return;
	}
	_endPoints.erase(endPoint);
	
	UM_LOG_INFO("%s: Removing endpoint at %s",
	            SHORT_UUID(_uuid).c_str(), endPoint.getAddress().c_str());

	COMMON_VARS;
	std::stringstream otherAddress;
	otherAddress << endPoint.getTransport() << "://" << endPoint.getIP() << ":" << endPoint.getPort();

	PREPARE_MSG(removeEndPointMsg, 4 + otherAddress.str().length() + 1);
	writePtr = writeVersionAndType(writePtr, Message::UM_DISCONNECT);
	assert(writePtr - writeBuffer == 4);
	writePtr = writeString(writePtr, otherAddress.str().c_str(), otherAddress.str().length());
	assert(writePtr - writeBuffer == 4 + otherAddress.str().length() + 1);

	zmq_sendmsg(_writeOpSocket, &removeEndPointMsg, 0) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
	zmq_msg_close(&removeEndPointMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

}

void ZeroMQNode::changed(ENDPOINT_RS_TYPE endPoint, uint64_t what) {
	if (what & Discovery::IFACE_REMOVED) {
		UM_LOG_INFO("%s gone on some interface -> removing and readding endpoint (be more clever here)", SHORT_UUID(_uuid).c_str());
		removed(endPoint);
		added(endPoint);
	}
}

/**
 * Process messages sent to our node socket.
 */
void ZeroMQNode::processNodeComm() {
	UM_TRACE("processNodeComm");
	COMMON_VARS;

#if 0
	for(;;) {
		RECV_MSG(_nodeSocket, msg);
		printf("%s: node socket received %d bytes from %s\n", _uuid.c_str(), msgSize, readPtr);
		zmq_msg_close(&msg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		MORE_OR_RETURN(_nodeSocket, "");
	}
#endif

	// read first message
	RECV_MSG(_nodeSocket, header);
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgRcvd++;
		_buckets.back().sizeMetaMsgRcvd += msgSize;
	}

	std::string from(recvBuffer, msgSize);
	zmq_msg_close(&header) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

	if (from.length() == 36) {
		RECV_MSG(_nodeSocket, content);

		// remember node and update last seen
		//touchNeighbor(from);

		// dealer socket sends no delimiter, but req does
		if (REMAINING_BYTES_TOREAD == 0) {
			zmq_getsockopt(_nodeSocket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
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
			zmq_msg_close(&content) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
			return;
		}

		Message::Type type;
		uint16_t version;
		readPtr = readVersionAndType(recvBuffer, version, type);

		if (version != Message::UM_VERSION) {
			UM_LOG_INFO("%s: node socket received unversioned or different message format version - discarding", SHORT_UUID(_uuid).c_str());
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
			if (from != _uuid || _allowLocalConns)
				processConnectedFrom(from);

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

			PublisherStubImpl* pubImpl = new PublisherStubImpl();
			SubscriberStubImpl* subImpl = new SubscriberStubImpl();

			readPtr = readSubInfo(readPtr, REMAINING_BYTES_TOREAD, subImpl);
			readPtr = readPubInfo(readPtr, REMAINING_BYTES_TOREAD, pubImpl);

			std::string pubUUID = pubImpl->getUUID();
			std::string subUUID = subImpl->getUUID();
			std::string address;

			delete pubImpl;

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
					_subscriptions[subUUID].subStub = SubscriberStub(SharedPtr<SubscriberStubImpl>(subImpl));
				} else {
					delete subImpl;
				}
				_subscriptions[subUUID].nodeUUID = from;
				_subscriptions[subUUID].address = address;
				_subscriptions[subUUID].pending[pubUUID] = _pubs[pubUUID];

				if (_subscriptions[subUUID].isZMQConfirmed || _subscriptions[subUUID].subStub.getImpl()->implType != Subscriber::ZEROMQ)
					confirmSub(subUUID);
			} else {
				delete subImpl;
				// remove a subscription
				Subscription& confSub = _subscriptions[subUUID];
				if (_connFrom.find(confSub.nodeUUID) != _connFrom.end() && _connFrom[confSub.nodeUUID]->node)
					_connFrom[confSub.nodeUUID]->node.removeSubscriber(confSub.subStub);

				// move all pending subscriptions to confirmed
				std::map<std::string, Publisher>::iterator confPubIter = confSub.confirmed.begin();
				while(confPubIter != confSub.confirmed.end()) {
					confPubIter->second.removed(confSub.subStub, _connFrom[confSub.nodeUUID]->node);
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

	} else {
		UM_LOG_WARN("%s: Got unprefixed message on _nodeSocket - discarding", SHORT_UUID(_uuid).c_str());
	}
}

void ZeroMQNode::processPubComm() {
	UM_TRACE("processPubComm");
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
		if (UUID::isUUID(subChannel.substr(1, zmq_msg_size(&message) - 2))) {
			subUUID = subChannel.substr(1, zmq_msg_size(&message) - 2);
		}

		if (subscription) {
			UM_LOG_INFO("%s: Got 0MQ subscription on %s", _uuid.c_str(), subChannel.c_str());
			if (subUUID.length() > 0) {
				// every subscriber subscribes to its uuid prefixed with a "~" for late alphabetical order
				_subscriptions[subUUID].isZMQConfirmed = true;
				if (_subscriptions[subUUID].subStub)
					confirmSub(subUUID);
			}
		} else {
			UM_LOG_INFO("%s: Got 0MQ unsubscription on %s", _uuid.c_str(), subChannel.c_str());
			if (subUUID.size() && _subscriptions[subUUID].isZMQConfirmed) {
				std::map<std::string, Publisher>::iterator pubIter = _subscriptions[subUUID].confirmed.begin();
				while(pubIter != _subscriptions[subUUID].confirmed.end()) {
					if(_connFrom.find(_subscriptions[subUUID].nodeUUID)!=_connFrom.end())
						pubIter->second.removed(_subscriptions[subUUID].subStub, _connFrom[_subscriptions[subUUID].nodeUUID]->node);
					else if(_connTo.find(_subscriptions[subUUID].nodeUUID)!=_connTo.end())
						pubIter->second.removed(_subscriptions[subUUID].subStub, _connTo[_subscriptions[subUUID].nodeUUID]->node);
					pubIter++;
				}
			}
		}

		zmq_getsockopt (_pubSocket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
		zmq_msg_close (&message) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		assert(!more); // subscriptions are not multipart
		if (!more)
			break;      //  Last message part
	}

}

/**
 * Process messages sent to one of the client sockets from a remote node
 */
void ZeroMQNode::processClientComm(SharedPtr<NodeConnection> client) {
	UM_TRACE("processClientComm");
	COMMON_VARS;

	// we have a reply from the server
	RECV_MSG(client->socket, opMsg);

	if (REMAINING_BYTES_TOREAD < 4) {
		zmq_msg_close(&opMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
		return;
	}

	Message::Type type;
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

		char* from;
		readPtr = readString(readPtr, from, 37);

		PublisherStubImpl* pubStub = new PublisherStubImpl();
		readPtr = readPubInfo(readPtr, REMAINING_BYTES_TOREAD, pubStub);
		assert(REMAINING_BYTES_TOREAD == 0);

		if (type == Message::UM_PUB_ADDED) {
			processRemotePubAdded(from, pubStub);
		} else {
			processRemotePubRemoved(from, pubStub);
		}

	}
	case Message::UM_SHUTDOWN: {
		// a remote neighbor shut down
		if (REMAINING_BYTES_TOREAD < 37) {
			break;
		}

		char* from;
		readPtr = readString(readPtr, from, 37);
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

		char* fromUUID;
		readPtr = readString(readPtr, fromUUID, 37);

		assert(client->address.length() > 0);

		if (fromUUID == _uuid && !_allowLocalConns) // do not connect to ourself
			break;

		processConnectedTo(fromUUID, client);
		processNodeInfo(recvBuffer + 4, msgSize - 4);

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
 */
void ZeroMQNode::processOpComm() {
	UM_TRACE("processOpComm");
	COMMON_VARS;

	// read first message
	RECV_MSG(_readOpSocket, opMsg)

	Message::Type type;
	uint16_t version;

	readPtr = readVersionAndType(recvBuffer, version, type);

	UM_LOG_INFO("%s: internal op socket received %s", SHORT_UUID(_uuid).c_str(), Message::typeToString(type));
	switch (type) {
	case Message::UM_PUB_REMOVED:
	case Message::UM_PUB_ADDED: {
		// removePublisher / addPublisher called us
		char* uuid;

		readPtr = readString(readPtr, uuid, 37);

		PublisherStubImpl* pubStub = new PublisherStubImpl();
		readPtr = readPubInfo(readPtr, REMAINING_BYTES_TOREAD, pubStub);
		assert(REMAINING_BYTES_TOREAD == 0);

		std::string internalPubId("inproc://um.pub.intern.");
		internalPubId += pubStub->getUUID();

		delete pubStub;

		if (type == Message::UM_PUB_ADDED) {
			zmq_connect(_subSocket, internalPubId.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		} else {
			zmq_disconnect(_subSocket, internalPubId.c_str()) && UM_LOG_ERR("zmq_disconnect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		}

		// tell every other node that we changed publishers
		NODE_BROADCAST_MSG(opMsg);

		break;
	}
	case Message::UM_DISCONNECT: {
		// endpoint was removed
		char* address;
		readPtr = readString(readPtr, address, msgSize - (readPtr - recvBuffer));

		if (_connTo.find(address) == _connTo.end())
			return;

		_connTo[address]->refCount--;

		if (_connTo[address]->refCount <= 0 && _connTo[address]->node) {
			NodeStub& nodeStub = _connTo[address]->node;
			std::string nodeUUID = nodeStub.getUUID();
			disconnectRemoteNode(nodeStub);

			// disconnect socket and remove as a neighbors
			zmq_close(_connTo[address]->socket);
			_connTo[address]->socket = NULL;

			// delete NodeConnection object if not contained in _connFrom as well
			if (!_connTo[address]->connectedFrom) {
				//delete _connTo[address];
			} else {
				assert(nodeUUID.size() > 0);
				assert(_connFrom.find(nodeUUID) != _connFrom.end());

				//delete _connFrom[nodeUUID];
				_connFrom.erase(nodeUUID);

			}
			_connTo.erase(address);

			if (nodeUUID.length() > 0)
				_connTo.erase(nodeUUID);

			_dirtySockets = true;
		}

		break;
	}
	case Message::UM_CONNECT_REQ: {
		// added(EndPoint) called us - rest of message is endpoint address
		char* address;
		readPtr = readString(readPtr, address, msgSize - (readPtr - recvBuffer));

		SharedPtr<NodeConnection> clientConn;
		// we don't know this endpoint
		if (_connTo.find(address) == _connTo.end()) {
			// open a new client connection
			clientConn = SharedPtr<NodeConnection>(new NodeConnection(address, _uuid));
			if (!clientConn->socket) {
//				delete clientConn;
				break;
			}
			_connTo[address] = clientConn;
		} else {
			clientConn = _connTo[address];
		}
		_dirtySockets = true;

		UM_LOG_INFO("%s: Sending CONNECT_REQ to %s", SHORT_UUID(_uuid).c_str(), address);

		// send a CONNECT_REQ message
		PREPARE_MSG(connReqMsg, 4);
		writePtr = writeVersionAndType(writePtr, Message::UM_CONNECT_REQ);
		assert(writePtr - writeBuffer == zmq_msg_size(&connReqMsg));

		zmq_sendmsg(clientConn->socket, &connReqMsg, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_sendmsg: %s", zmq_strerror(errno));
		if (_buckets.size() > 0) {
			_buckets.back().nrMetaMsgSent++;
			_buckets.back().sizeMetaMsgSent += 4;
		}
		zmq_msg_close(&connReqMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

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

void ZeroMQNode::updateSockets() {
	RScopeLock lock(_mutex);
	UM_TRACE("updateSockets");

	if (_sockets != NULL)
		free(_sockets);
	_nodeSockets.clear();

	size_t maxSockets = _connTo.size() + _nrStdSockets;
	_sockets = (zmq_pollitem_t*)malloc(sizeof(zmq_pollitem_t) * maxSockets);
	// prepopulate with standard sockets
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
	size_t more_size = sizeof(more);

	while(isStarted()) {
		// make sure we have a bucket for performance measuring
		if (_buckets.size() == 0)
			_buckets.push_back(StatBucket<size_t>());

		if (_dirtySockets)
			updateSockets();

		for (int i = 0; i < _nrSockets; i++) {
			_sockets[i].revents = 0;
		}

		//UM_LOG_DEBUG("%s: polling on %ld sockets", _uuid.c_str(), nrSockets);
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
					processClientComm(_connTo[nodeSockIter->second]);
				} else {
					UM_LOG_WARN("%s: message from vanished node %s", _uuid.c_str(), nodeSockIter->second.c_str());
				}
				break;
			}
			nodeSockIter++;
		}


		if (_sockets[0].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			processNodeComm();
			DRAIN_SOCKET(_nodeSocket);
		}

		if (_sockets[1].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			processPubComm();
			DRAIN_SOCKET(_pubSocket);
		}

		if (_sockets[2].revents & ZMQ_POLLIN) {
			RScopeLock lock(_mutex);
			processOpComm();
			DRAIN_SOCKET(_readOpSocket);
		}

		// someone is publishing - this is last to
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
				zmq_getsockopt (_subSocket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
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

	std::map<std::string, SharedPtr<NodeConnection> >::iterator connIter = _connFrom.begin();
	while(connIter != _connFrom.end()) {
		if (!UUID::isUUID(connIter->first)) {
			connIter++;
			continue;
		}
		if (now - connIter->second->node.getLastSeen() > 30000) {
			// we have been very patient remove pending node
			UM_LOG_ERR("%s timeout for %s - removing", SHORT_UUID(_uuid).c_str(), SHORT_UUID(connIter->second->node.getUUID()).c_str());
			NodeStub nodeStub = connIter->second->node;
			// Disconnect subscribers
			std::map<std::string, PublisherStub> pubStubs = nodeStub.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubStubIter = pubStubs.begin();
			while(pubStubIter != pubStubs.end()) {
				std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
				while(subIter != _subs.end()) {
					if (subIter->second.matches(pubStubIter->second.getChannelName()))
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

void ZeroMQNode::disconnectRemoteNode(NodeStub& nodeStub) {
	UM_TRACE("disconnectRemoteNode");
	if (!nodeStub)
		return;

	std::string nodeUUID = nodeStub.getUUID();
	std::map<std::string, PublisherStub> remotePubs = nodeStub.getPublishers();
	std::map<std::string, PublisherStub>::iterator remotePubIter = remotePubs.begin();
	std::map<std::string, Subscriber>::iterator localSubIter = _subs.begin();

	// iterate all remote publishers and remove from local subs
	while (remotePubIter != remotePubs.end()) {
		while (localSubIter != _subs.end()) {
			if(localSubIter->second.matches(remotePubIter->second.getChannelName())) {
				localSubIter->second.removed(remotePubIter->second, nodeStub);
				sendSubRemoved(nodeStub.getUUID().c_str(), localSubIter->second, remotePubIter->second);
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

	std::map<std::string, SubscriberStub> remoteSubs = nodeStub.getSubscribers();
	std::map<std::string, SubscriberStub>::iterator remoteSubIter = remoteSubs.begin();
	while(remoteSubIter != remoteSubs.end()) {
		if (_subscriptions.find(remoteSubIter->first) != _subscriptions.end()) {
			Subscription& subscription = _subscriptions[remoteSubIter->first];
			std::map<std::string, Publisher>::iterator confirmedIter = subscription.confirmed.begin();
			while(confirmedIter != subscription.confirmed.end()) {
				confirmedIter->second.removed(subscription.subStub, nodeStub);
				confirmedIter++;
			}
			_subscriptions.erase(remoteSubIter->first);
		} else {
			// could not find any subscriptions
		}
		remoteSubIter++;
	}
}

void ZeroMQNode::processConnectedFrom(const std::string& uuid) {
	UM_TRACE("processConnectedFrom");
	// we received a connection from the given uuid

	// let's see whether we already are connected *to* this one
	if (_connTo.find(uuid) != _connTo.end()) {
		_connFrom[uuid] = _connTo[uuid];
	} else {
		SharedPtr<NodeConnection> conn = SharedPtr<NodeConnection>(new NodeConnection());
		conn->node = NodeStub(SharedPtr<NodeStubImpl>(new NodeStubImpl()));
		conn->node.getImpl()->setUUID(uuid);
		_connFrom[uuid] = conn;
	}

	// in any case, mark as connected from and update last seen
	_connFrom[uuid]->connectedFrom = true;
	_connFrom[uuid]->node.updateLastSeen();
}

void ZeroMQNode::processConnectedTo(const std::string& uuid, SharedPtr<NodeConnection> client) {
	UM_TRACE("processConnectedTo");
	assert(client->address.length() > 0);

	// parse client's address back into its constituting parts
	size_t colonPos = client->address.find_last_of(":");
	if(colonPos == std::string::npos || client->address.length() < 6 + 8 + 1 + 1) {
		return;
	}

	std::string transport = client->address.substr(0,3);
	std::string ip = client->address.substr(6, colonPos - 6);
	std::string port = client->address.substr(colonPos + 1, client->address.length() - colonPos + 1);

	// processOpComm must have added this one
	assert(_connTo.find(client->address) != _connTo.end());

	NodeStub nodeStub;

	if (client->node && uuid != client->node.getUUID()) {
		// previous and this uuid of remote node differ - assume that it was replaced
		disconnectRemoteNode(client->node);
		_connTo.erase(client->node.getUUID());
		_connFrom.erase(client->node.getUUID());
		nodeStub = NodeStub(SharedPtr<NodeStubImpl>(new NodeStubImpl()));
		client->refCount = 0;
	}

	// are we already connected *from* this one?
	if (_connFrom.find(uuid) != _connFrom.end()) {
		nodeStub = _connFrom[uuid]->node; // maybe we remembered something about the stub, keep entry
		_connFrom[uuid] = client;   // overwrite with this one
		client->connectedFrom = true; // and remember that we are already connected from
	} else {
		// we know nothing about the remote node yet
		nodeStub = NodeStub(SharedPtr<NodeStubImpl>(new NodeStubImpl()));
	}

	assert(_connTo.find(client->address) != _connTo.end());

	// remember remote node
	client->refCount++;
	client->connectedTo = true;
	client->node = nodeStub;
	client->node.getImpl()->setUUID(uuid);
	client->node.getImpl()->setTransport(transport);
	client->node.getImpl()->setIP(ip);
	client->node.getImpl()->setPort(strTo<uint16_t>(port));
	client->node.updateLastSeen();
	_connTo[uuid] = client;
	_dirtySockets = true;

}

void ZeroMQNode::confirmSub(const std::string& subUUID) {
	UM_TRACE("confirmSub");
	if (_subscriptions.find(subUUID) == _subscriptions.end())
		return;

	if (!_subscriptions[subUUID].subStub)
		return;

	if (_subscriptions[subUUID].subStub.getImpl()->implType == Subscriber::ZEROMQ &&
	        !_subscriptions[subUUID].isZMQConfirmed)
		return;

	Subscription& pendSub = _subscriptions[subUUID];

	if (_connFrom.find(pendSub.nodeUUID) != _connFrom.end()) {
		_connFrom[pendSub.nodeUUID]->node.addSubscriber(pendSub.subStub);
	}

	// copy over address from getpeer*
	if (_connFrom[pendSub.nodeUUID]->node.getIP().length() == 0 && pendSub.address.length() > 0)
		_connFrom[pendSub.nodeUUID]->node.getImpl()->setIP(pendSub.address);

	// move all pending subscriptions to confirmed
	std::map<std::string, Publisher>::iterator pendPubIter = pendSub.pending.begin();
	while(pendPubIter != pendSub.pending.end()) {
		pendPubIter->second.added(pendSub.subStub, _connFrom[pendSub.nodeUUID]->node);
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
	writePtr = writeString(writePtr, _uuid.c_str(), _uuid.length());
	assert(writePtr - writeBuffer == 4 + _uuid.length() + 1);

	pubIter =_pubs.begin();
	while(pubIter != _pubs.end()) {
		writePtr = writePubInfo(writePtr, pubIter->second);
		pubIter++;
	}

	assert(writePtr - writeBuffer == zmq_msg_size(msg));
}

void ZeroMQNode::processNodeInfo(char* recvBuffer, size_t msgSize) {
	UM_TRACE("processNodeInfo");

	char* readPtr = recvBuffer;
	if(REMAINING_BYTES_TOREAD < 36)
		return;

	char* from;
	readPtr = readString(readPtr, from, 37);

	if (_connTo.find(from) == _connTo.end()) {
		UM_LOG_WARN("%s not caring for nodeinfo from %s - not connected", SHORT_UUID(_uuid).c_str(), from);
		return;
	}

	std::set<std::string> pubUUIDs;
	while(REMAINING_BYTES_TOREAD > 37) {

		PublisherStubImpl* pubStub = new PublisherStubImpl();
		readPtr = readPubInfo(readPtr, REMAINING_BYTES_TOREAD, pubStub);
		processRemotePubAdded(from, pubStub);
	}
}

void ZeroMQNode::processRemotePubAdded(char* nodeUUID, PublisherStubImpl* pub) {
	UM_TRACE("processRemotePubAdded");

	if (_connTo.find(nodeUUID) == _connTo.end()) {
		delete pub;
		return;
	}

	NodeStub nodeStub = _connTo[nodeUUID]->node;
	nodeStub.updateLastSeen();

	PublisherStub pubStub((SharedPtr<PublisherStubImpl>(pub)));
	pubStub.getImpl()->setDomain(nodeUUID);

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
		if (subIter->second.getImpl()->implType == pubStub.getImpl()->implType && subIter->second.matches(pubStub.getChannelName())) {
			subIter->second.added(pubStub, nodeStub);
			sendSubAdded(nodeUUID, subIter->second, pubStub);
		}
		subIter++;
	}
}

void ZeroMQNode::processRemotePubRemoved(char* nodeUUID, PublisherStubImpl* pub) {
	UM_TRACE("processRemotePubRemoved");

	if (_connTo.find(nodeUUID) == _connTo.end())
		return;

	NodeStub nodeStub = _connTo[nodeUUID]->node;
	PublisherStub pubStub = nodeStub.getPublisher(pub->getUUID());

	if (!pubStub) {
		delete pub;
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
		if (subIter->second.getImpl()->implType == pubStub.getImpl()->implType && subIter->second.matches(pubStub.getChannelName())) {
			subIter->second.removed(pubStub, nodeStub);
			sendSubRemoved(nodeUUID, subIter->second, pubStub);
		}
		subIter++;
	}
	nodeStub.removePublisher(pubStub);
	delete pub;
}

void ZeroMQNode::sendSubAdded(const char* nodeUUID, const Subscriber& sub, const PublisherStub& pub) {
	UM_TRACE("sendSubAdded");
	COMMON_VARS;

	if (_connTo.find(nodeUUID) == _connTo.end())
		return;

	void* clientSocket = _connTo[nodeUUID]->socket;
	if (!clientSocket)
		return;

	UM_LOG_INFO("Sending sub added for %s on %s to publisher %s",
	            sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());

	size_t bufferSize = 4 + SUB_INFO_SIZE(sub) + PUB_INFO_SIZE(pub);
	PREPARE_MSG(subAddedMsg, bufferSize);

	writePtr = writeVersionAndType(writePtr, Message::UM_SUBSCRIBE);
	writePtr = writeSubInfo(writePtr, sub);
	writePtr = writePubInfo(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&subAddedMsg, clientSocket, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}
	zmq_msg_close(&subAddedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
}

void ZeroMQNode::sendSubRemoved(const char* nodeUUID, const Subscriber& sub, const PublisherStub& pub) {
	UM_TRACE("sendSubRemoved");
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
	writePtr = writeSubInfo(writePtr, sub);
	writePtr = writePubInfo(writePtr, pub);
	assert(writePtr - writeBuffer == bufferSize);

	zmq_msg_send(&subRemovedMsg, clientSocket, ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_msg_send: %s", zmq_strerror(errno));
	if (_buckets.size() > 0) {
		_buckets.back().nrMetaMsgSent++;
		_buckets.back().sizeMetaMsgSent += bufferSize;
	}
	zmq_msg_close(&subRemovedMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));

}

char* ZeroMQNode::writePubInfo(char* buffer, const PublisherStub& pub) {
	std::string channel = pub.getChannelName();
	std::string uuid = pub.getUUID(); // we share a publisher socket in the node, do not publish hidden pub uuid
	uint16_t port = pub.getPort();
	uint16_t type = pub.getImpl()->implType;
	if(type == ZMQ_PUB)
		port=_pubPort;

	assert(uuid.length() == 36);

	char* start = buffer;
	(void)start; // surpress unused warning wiithout assert

	buffer = writeString(buffer, channel.c_str(), channel.length());
	buffer = writeString(buffer, uuid.c_str(), uuid.length());
	buffer = writeUInt16(buffer, type);
	buffer = writeUInt16(buffer, port);

	assert(buffer - start == PUB_INFO_SIZE(pub));
	return buffer;
}

char* ZeroMQNode::readPubInfo(char* buffer, size_t available, PublisherStubImpl* pubStub) {
	char* start = buffer;
	(void)start; // surpress unused warning without assert

	char* channelName;
	buffer = readString(buffer, channelName, available - (buffer - start));
	pubStub->setChannelName(channelName);

	char* uuid;
	buffer = readString(buffer, uuid, available - (buffer - start));
	pubStub->setUUID(uuid);

	uint16_t type;
	buffer = readUInt16(buffer, type);
	pubStub->implType = type;

	uint16_t port;
	buffer = readUInt16(buffer, port);
	pubStub->setPort(port);

	return buffer;
}

char* ZeroMQNode::writeSubInfo(char* buffer, const Subscriber& sub) {
	assert(sub.getUUID().length() == 36);

	char* start = buffer;
	(void)start; // surpress unused warning without assert

	buffer = writeString(buffer, sub.getChannelName().c_str(), sub.getChannelName().length());
	buffer = writeString(buffer, sub.getUUID().c_str(), sub.getUUID().length());
	buffer = writeUInt16(buffer, sub.getImpl()->implType);
	buffer = writeUInt16(buffer, sub.isMulticast() ? 1 : 0);
	buffer = writeString(buffer, sub.getIP().c_str(), sub.getIP().length());
	buffer = writeUInt16(buffer, sub.getPort());

	assert(buffer - start == SUB_INFO_SIZE(sub));
	return buffer;
}

char* ZeroMQNode::readSubInfo(char* buffer, size_t available, SubscriberStubImpl* subStub) {
	char* start = buffer;
	(void)start; // surpress unused warning without assert

	char* channelName;
	buffer = readString(buffer, channelName, available - (buffer - start));
	subStub->setChannelName(channelName);

	char* uuid;
	buffer = readString(buffer, uuid, available - (buffer - start));
	subStub->setUUID(uuid);

	uint16_t type;
	buffer = readUInt16(buffer, type);
	subStub->implType = type;

	uint16_t multicast;
	buffer = readUInt16(buffer, multicast);
	subStub->setMulticast(multicast==1 ? true : false);

	char* ip;
	buffer = readString(buffer, ip, available - (buffer - start));
	subStub->setIP(ip);

	uint16_t port;
	buffer = readUInt16(buffer, port);
	subStub->setPort(port);

	return buffer;
}

char* ZeroMQNode::writeVersionAndType(char* buffer, Message::Type type) {
	buffer = writeUInt16(buffer, Message::UM_VERSION);
	buffer = writeUInt16(buffer, type);
	return buffer;
}

char* ZeroMQNode::readVersionAndType(char* buffer, uint16_t& version, Message::Type& type) {
	// clear out of type bytes
	memset(&type, 0, sizeof(type));

	buffer = readUInt16(buffer, version);
	buffer = readUInt16(buffer, (uint16_t&)type);
	return buffer;
}

char* ZeroMQNode::writeString(char* buffer, const char* content, size_t length) {
	memcpy(buffer, content, length);
	buffer += length;
	buffer[0] = 0;
	buffer += 1;
	return buffer;
}

char* ZeroMQNode::readString(char* buffer, char*& content, size_t maxLength) {
	if (strnlen(buffer, maxLength) < maxLength) {
		content = buffer;
		buffer += strlen(content) + 1;
	} else {
		content = NULL;
	}
	return buffer;
}

char* ZeroMQNode::writeUInt16(char* buffer, uint16_t value) {
	*(uint16_t*)(buffer) = htons(value);
	buffer += 2;
	return buffer;
}

char* ZeroMQNode::readUInt16(char* buffer, uint16_t& value) {
	value = ntohs(*(uint16_t*)(buffer));
	buffer += 2;
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
	ss << "uuid:" << _uuid;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	ss << "host:" << hostUUID;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

#ifdef APPLE
	ss << "os: MacOSX";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#elif defined CYGWIN
	ss << "os: Cygwin";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#elif defined ANDROID
	ss << "os: Android";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#elif defined IOS
	ss << "os: iOS";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#elif defined UNIX
	ss << "os: Unix";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#elif defined WIN32
	ss << "os: Windows";
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
#endif
	RESETSS(ss);

	ss << "proc:" << procUUID;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	// calculate performance

	ss << "sent:msgs:" << statBucket.nrMetaMsgSent;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	ss << "sent:bytes:" << statBucket.sizeMetaMsgSent;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	ss << "rcvd:msgs:" << statBucket.nrMetaMsgRcvd;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	ss << "rcvd:bytes:" << statBucket.sizeMetaMsgRcvd;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

	// send our publishers
	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while (pubIter != _pubs.end()) {

		ss << "pub:uuid:" << pubIter->first;
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		ss << "pub:channelName:" << pubIter->second.getChannelName();
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		ss << "pub:type:" << pubIter->second.getImpl()->implType;
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

//		std::cout << pubIter->second.getChannelName() << std::endl;
//		std::cout << statBucket.nrChannelMsg[pubIter->second.getChannelName()] << std::endl;

		ss << "pub:sent:msgs:" << statBucket.nrChannelMsg[pubIter->second.getChannelName()];
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		ss << "pub:sent:bytes:" << statBucket.sizeChannelMsg[pubIter->second.getChannelName()];
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		std::map<std::string, SubscriberStub> subs = pubIter->second.getSubscribers();
		std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();

		// send its subscriptions as well
		while (subIter != subs.end()) {
			ss << "pub:sub:uuid:" << subIter->first;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "pub:sub:channelName:" << subIter->second.getChannelName();
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "pub:sub:type:" << subIter->second.getImpl()->implType;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			subIter++;
		}
		pubIter++;
	}

	// send our subscribers
	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while (subIter != _subs.end()) {
		ss << "sub:uuid:" << subIter->first;
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		ss << "sub:channelName:" << subIter->second.getChannelName();
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		ss << "sub:type:" << subIter->second.getImpl()->implType;
		zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
		RESETSS(ss);

		std::map<std::string, PublisherStub> pubs = subIter->second.getPublishers();
		std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
		// send all remote publishers we think this node has
		while (pubIter != pubs.end()) {
			ss << "sub:pub:uuid:" << pubIter->first;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "sub:pub:channelName:" << pubIter->second.getChannelName();
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "sub:pub:type:" << pubIter->second.getImpl()->implType;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			pubIter++;
		}

		subIter++;
	}


	// send all the nodes we know about
	std::map<std::string, SharedPtr<NodeConnection> > connections;
	std::map<std::string, SharedPtr<NodeConnection> >::const_iterator nodeIter;

	// insert connection to
	nodeIter = _connTo.begin();
	while (_connTo.size() > 0 && nodeIter != _connTo.end()) {
		connections.insert(*nodeIter);
		nodeIter++;
	}

	// insert connection to
	nodeIter = _connFrom.begin();
	while (nodeIter != _connFrom.end()) {
		if (connections.find(nodeIter->first) != connections.end()) {
			assert(connections[nodeIter->first] == nodeIter->second);
		}
		connections.insert(*nodeIter);
		nodeIter++;
	}

	nodeIter = connections.begin();
	while (nodeIter != connections.end()) {
		// only report UUIDs as keys
		if (UUID::isUUID(nodeIter->first)) {
			ss << "conn:uuid:" << nodeIter->first;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:address:" << nodeIter->second->address;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:from:" << nodeIter->second->connectedFrom;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:to:" << nodeIter->second->connectedTo;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:confirmed:" << nodeIter->second->isConfirmed;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:refCount:" << nodeIter->second->refCount;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			ss << "conn:startedAt:" << nodeIter->second->startedAt;
			zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
			RESETSS(ss);

			// send info about all nodes connected to us
			if (nodeIter->second->node) {
				NodeStub& nodeStub = nodeIter->second->node;

				ss << "conn:lastSeen:" << nodeStub.getLastSeen();
				zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
				RESETSS(ss);

				std::map<std::string, PublisherStub> pubs = nodeStub.getPublishers();
				std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
				// send all remote publishers we think this node has
				while (pubIter != pubs.end()) {
					ss << "conn:pub:uuid:" << pubIter->first;
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					ss << "conn:pub:channelName:" << pubIter->second.getChannelName();
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					ss << "conn:pub:type:" << pubIter->second.getImpl()->implType;
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					pubIter++;
				}

				std::map<std::string, SubscriberStub> subs = nodeStub.getSubscribers();
				std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
				// send all remote publishers we think this node has
				while (subIter != subs.end()) {
					ss << "conn:sub:uuid:" << subIter->first;
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					ss << "conn:sub:channelName:" << subIter->second.getChannelName();
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					ss << "conn:sub:type:" << subIter->second.getImpl()->implType;
					zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_SNDMORE | ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
					RESETSS(ss);

					subIter++;
				}

			}
		}
		nodeIter++;
	}

	ss << "done:" << _uuid;
	zmq_send(_nodeSocket, ss.str().c_str(), ss.str().length(), ZMQ_DONTWAIT) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));
	RESETSS(ss);

}

ZeroMQNode::NodeConnection::NodeConnection() :
	connectedTo(false),
	connectedFrom(false),
	socket(NULL),
	startedAt(0),
	refCount(0), isConfirmed(false) {
	UM_TRACE("NodeConnection");
}

ZeroMQNode::NodeConnection::NodeConnection(const std::string& _address, const std::string& thisUUID) :
	connectedTo(false),
	connectedFrom(false),
	address(_address),
	refCount(0),
	isConfirmed(false) {
	UM_TRACE("NodeConnection");
	startedAt	= Thread::getTimeStampMs();
	socketId = thisUUID;
	socket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_DEALER);
	if (!socket) {
		UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	} else {
		zmq_setsockopt(socket, ZMQ_IDENTITY, socketId.c_str(), socketId.length()) && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
		int err = zmq_connect(socket, address.c_str());
		if (err) {
			UM_LOG_ERR("zmq_connect %s: %s", address.c_str(), zmq_strerror(errno));
			zmq_close(socket);
			socket = NULL;
		}
	}
}

ZeroMQNode::NodeConnection::~NodeConnection() {
	UM_TRACE("~NodeConnection");
	if (socket) {
		zmq_disconnect(socket, address.c_str());
		zmq_close(socket);
	}
}

ZeroMQNode::Subscription::Subscription() :
	isZMQConfirmed(false),
	startedAt(Thread::getTimeStampMs()) {
	UM_TRACE("Subscription");
}

}
