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

#include "umundo/config.h"

#if defined UNIX || defined IOS || defined IOSSIM
#include <arpa/inet.h> // htons
#include <string.h> // strlen, memcpy
#include <stdio.h> // snprintf
#endif

#include <boost/lexical_cast.hpp>

#include "umundo/common/Message.h"
#include "umundo/common/Regex.h"
#include "umundo/common/UUID.h"
#include "umundo/discovery/Discovery.h"
#include "umundo/discovery/NodeQuery.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

namespace umundo {

void* ZeroMQNode::getZeroMQContext() {
	if (_zmqContext == NULL) {
		(_zmqContext = zmq_ctx_new()) || LOG_ERR("zmq_init: %s",zmq_strerror(errno));
	}
	return _zmqContext;
}
void* ZeroMQNode::_zmqContext = NULL;

boost::shared_ptr<Implementation> ZeroMQNode::create() {
	return boost::shared_ptr<ZeroMQNode>(new ZeroMQNode());
}

ZeroMQNode::ZeroMQNode() {
}

void ZeroMQNode::init(boost::shared_ptr<Configuration>) {
	_transport = "tcp";

	(_pubSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_XPUB))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_subSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_SUB))     || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_nodeSocket    = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_ROUTER))  || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_readOpSocket  = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_writeOpSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_bind(_readOpSocket, readOpId.c_str())  && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
	zmq_connect(_writeOpSocket, readOpId.c_str()) && LOG_ERR("zmq_connect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	int hwm = 5000000;
	int vbsSub = 1;

	std::string pubId("um.pub." + _uuid);
	zmq_setsockopt(_pubSocket, ZMQ_IDENTITY, pubId.c_str(), pubId.length()) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_pubSocket, ZMQ_SNDHWM, &hwm, sizeof(hwm))               && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_pubSocket, ZMQ_XPUB_VERBOSE, &vbsSub, sizeof(vbsSub))   && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // receive all subscriptions

	zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, "", 0)          && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // subscribe to every internal publisher

	std::string nodeId("um.node." + _uuid);
	zmq_setsockopt(_nodeSocket, ZMQ_IDENTITY, nodeId.c_str(), nodeId.length()) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));

	_port = bindToFreePort(_nodeSocket, "tcp", "*");
	zmq_bind(_nodeSocket, std::string("inproc://" + nodeId).c_str()) && LOG_WARN("zmq_bind: %s", zmq_strerror(errno));
//  zmq_bind(_nodeSocket, std::string("ipc://" + nodeId).c_str())    && LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + nodeId).c_str(),  zmq_strerror(errno));

	_pubPort = bindToFreePort(_pubSocket, "tcp", "*");
	zmq_bind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
//  zmq_bind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + pubId).c_str(), zmq_strerror(errno))

	LOG_INFO("Node %s listening on port %d", SHORT_UUID(_uuid).c_str(), _port);
	LOG_INFO("Publisher %s listening on port %d", SHORT_UUID(_uuid).c_str(), _pubPort);

	_nodeQuery = boost::shared_ptr<NodeQuery>(new NodeQuery(_domain, this));

	Discovery::add(this);
	Discovery::browse(_nodeQuery);

	Thread::start();
}

ZeroMQNode::~ZeroMQNode() {

#if 0
	// remove all our subscribers and publshers first for a graceful exit
	std::map<std::string, Publisher>::iterator pubIter =  _pubs.begin();
	while(pubIter != _pubs.end()) {
		removePublisher(pubIter->second);
		pubIter++;
	}

	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		removeSubscriber(subIter->second);
		subIter++;
	}
#endif

	Discovery::unbrowse(_nodeQuery);
	Discovery::remove(this);

	Thread::stop();

	// close connection to all remote publishers
	std::map<std::string, void*>::iterator sockIter;
	for (sockIter = _sockets.begin(); sockIter != _sockets.end(); sockIter++) {
		zmq_close(sockIter->second) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	}

	join();
	zmq_close(_pubSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_subSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_nodeSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_readOpSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_writeOpSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::suspend() {
	ScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;

	// stop browsing for remote nodes
	Discovery::unbrowse(_nodeQuery);
	Discovery::remove(this);

};

void ZeroMQNode::resume() {
	ScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;

	std::map<std::string, Subscriber>::iterator localSubIter = _subs.begin();
	while(localSubIter != _subs.end()) {
		localSubIter->second.resume();
		localSubIter++;
	}

	Discovery::add(this);
	Discovery::browse(_nodeQuery);
};

uint16_t ZeroMQNode::bindToFreePort(void* socket, const std::string& transport, const std::string& address) {
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
			LOG_WARN("zmq_bind at %s: %s", ss.str().c_str(), zmq_strerror(errno));
			Thread::sleepMs(100);
		}
	}

	return port;
}

void ZeroMQNode::addSubscriber(Subscriber& sub) {
	ScopeLock lock(_mutex);
	if (_subs.find(sub.getUUID()) == _subs.end()) {
		_subs[sub.getUUID()] = sub;

		LOG_INFO("%s added subscriber %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

		std::map<std::string, NodeStub>::iterator nodeIter = _nodes.begin();
		while (nodeIter != _nodes.end()) {
			void* nodeSocket = _sockets[nodeIter->first];
			assert(nodeSocket != NULL);

			std::map<std::string, PublisherStub> pubs = nodeIter->second.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
			// iterate all remote publishers
			while (pubIter != pubs.end()) {
				if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
					sub.added(pubIter->second);
					sendSubAdded(nodeSocket, sub, pubIter->second, false);
				}
				pubIter++;
			}
			nodeIter++;
		}
	}
	assert(validateState());

}

void ZeroMQNode::removeSubscriber(Subscriber& sub) {
	ScopeLock lock(_mutex);
	if (_subs.find(sub.getUUID()) != _subs.end()) {
		std::string subId("inproc://um.sub." + sub.getUUID());
		_subs.erase(sub.getUUID());

		LOG_INFO("%s removed subscriber %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

		std::map<std::string, NodeStub>::iterator nodeIter = _nodes.begin();
		while (nodeIter != _nodes.end()) {
			void* nodeSocket = _sockets[nodeIter->first];
			assert(nodeSocket != NULL);

			std::map<std::string, PublisherStub> pubs = nodeIter->second.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
			// iterate all remote publishers
			while (pubIter != pubs.end()) {
				if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
					sendSubRemoved(nodeSocket, sub, pubIter->second, false);
					sub.removed(pubIter->second);
				}
				pubIter++;
			}
			nodeIter++;
		}
	}
	assert(validateState());

}

void ZeroMQNode::addPublisher(Publisher& pub) {
	ScopeLock lock(_mutex);
	if (_pubs.find(pub.getUUID()) == _pubs.end()) {
		std::string internalPubId("inproc://um.pub.intern." + pub.getUUID());
//    zmq_connect(_subSocket, internalPubId.c_str()) && LOG_ERR("zmq_connect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		ZMQ_INTERNAL_SEND("connectPub", internalPubId.c_str());

		LOG_INFO("%s added publisher %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

		_pubs[pub.getUUID()] = pub;
		std::map<std::string, void*>::iterator socketIter = _sockets.begin();
		// notify all other nodes
		while (socketIter != _sockets.end()) {
			sendPubAdded(socketIter->second, pub, false);
			socketIter++;
		}
	}
	assert(validateState());

}

void ZeroMQNode::removePublisher(Publisher& pub) {
	ScopeLock lock(_mutex);
	if (_pubs.find(pub.getUUID()) != _pubs.end()) {
		std::string internalPubId("inproc://um.pub.intern." + pub.getUUID());
//    zmq_disconnect(_subSocket, internalPubId.c_str()) && LOG_ERR("zmq_disconnect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
		ZMQ_INTERNAL_SEND("disconnectPub", internalPubId.c_str());

		LOG_INFO("%s removed publisher %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

		_pubs.erase(pub.getUUID());
		std::map<std::string, void*>::iterator socketIter = _sockets.begin();
		// notify all other nodes
		while (socketIter != _sockets.end()) {
			sendPubRemoved(socketIter->second, pub, false);
			socketIter++;
		}
	}
	assert(validateState());

}

void ZeroMQNode::added(NodeStub node) {
	// is this us?
	if (node.getUUID() == _uuid)
		return;
	ScopeLock lock(_mutex);

  LOG_INFO("Found new node %s at %s://%s:%d", node.getUUID().c_str(), node.getTransport().c_str(), node.getIP().c_str(), node.getPort());

	// embedded mDNS will not report removed nodes when exited non-gracefully
	map<string, NodeStub>::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		NodeStub nodeStub = nodeIter->second;
		nodeIter++;
		
		if (
			nodeStub.getPort() == node.getPort() &&
			nodeStub.getIP() == node.getIP()
		) {
			removed(nodeStub);
		}
	}

	if (_nodes.find(node.getUUID()) != _nodes.end()) {
		LOG_WARN("Not adding already known node");
		return;
	}

	// establish connection
	std::stringstream ss;
	if (node.isInProcess()) {
		// same process, use inproc communication
		ss << "inproc://um.node." << node.getUUID();
	} else if (!node.isRemote() && false) { // disabled for now
		// same host, use inter-process communication
		ss << "ipc://um.node." << node.getUUID();
	} else {
		// remote node, use network
		ss << node.getTransport() << "://" << node.getIP() << ":" << node.getPort();
	}

	// connect a socket to the remote node port
	void* client;
	(client = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_DEALER)) || LOG_ERR("zmq_socket: %s",zmq_strerror(errno));

	// give the socket an id for zeroMQ routing
	zmq_setsockopt(client, ZMQ_IDENTITY, _uuid.c_str(), _uuid.length()) && LOG_ERR("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_connect(client, ss.str().c_str()) && LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));

	// remember node stub and socket
	_nodes[node.getUUID()] = node;
	_sockets[node.getUUID()] = client;

	// send local publishers to remote node
	std::map<std::string, Publisher>::iterator pubIter;
	int hasMore = _pubs.size() - 1;
	for (pubIter = _pubs.begin(); pubIter != _pubs.end(); pubIter++, hasMore--) {
		// create a publisher added message from current publisher
		sendPubAdded(client, pubIter->second, (hasMore > 0));
	}
	LOG_INFO("sending %d publishers to newcomer", _pubs.size());

	if (_pendingRemotePubs.find(node.getUUID()) != _pendingRemotePubs.end()) {
		// the other node found us first and already sent its publishers
		std::map<std::string, PublisherStub>::iterator pubIter = _pendingRemotePubs[node.getUUID()].begin();
		while(pubIter != _pendingRemotePubs[node.getUUID()].end()) {
			confirmPubAdded(node, pubIter->second);
			pubIter++;
		}
		_pendingRemotePubs.erase(node.getUUID());
	}
	assert(validateState());

}

void ZeroMQNode::removed(NodeStub node) {
	// is this us?
	if (node.getUUID() == _uuid)
		return;
	ScopeLock lock(_mutex);

  LOG_INFO("Lost node %s", node.getUUID().c_str());

	if (_nodes.find(node.getUUID()) == _nodes.end()) {
		LOG_WARN("Not removing unknown node");
		return;
	}

	std::map<std::string, PublisherStub> pubs = _nodes[node.getUUID()].getPublishers();
	std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
	while (pubIter != pubs.end()) {
		processNodeCommPubRemoval(node.getUUID(), pubIter->second);
		pubIter++;
	}

	std::map<std::string, SubscriberStub> subs = _nodes[node.getUUID()].getSubscribers();
	std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
	while (subIter != subs.end()) {
		std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
		while (pubIter != _pubs.end()) {
			if(pubIter->second.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
				pubIter->second.removedSubscriber(node.getUUID(), subIter->second.getUUID());
			}
			pubIter++;
		}
		subIter++;
	}

	if (_sockets.find(node.getUUID()) == _sockets.end()) {
		LOG_WARN("removed: client was never added: %s", SHORT_UUID(node.getUUID()).c_str());
		return;
	}
	zmq_close(_sockets[node.getUUID()]) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));

	_sockets.erase(node.getUUID());               // delete socket
	_nodes.erase(node.getUUID());
	_pendingRemotePubs.erase(node.getUUID());
	assert(_sockets.size() == _nodes.size());

	LOG_INFO("%s removed %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(node.getUUID()).c_str());
	assert(validateState());

}

void ZeroMQNode::changed(NodeStub node) {
}

void ZeroMQNode::sendPubRemoved(void* socket, const Publisher& pub, bool hasMore) {
	zmq_msg_t msg;
	int pubInfoSize = PUB_INFO_SIZE(pub) + 2 + 2;
	ZMQ_PREPARE(msg, pubInfoSize);

	char* buffer = (char*)zmq_msg_data(&msg);
	*(uint16_t*)(buffer) = htons(Message::VERSION);
	buffer += 2;
	*(uint16_t*)(buffer) = htons(Message::PUB_REMOVED);
	buffer += 2;
	writePubInfo(buffer, pub);

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::sendPubAdded(void* socket, const Publisher& pub, bool hasMore) {
	zmq_msg_t msg;
	int pubInfoSize = PUB_INFO_SIZE(pub) + 2 + 2;
	ZMQ_PREPARE(msg, pubInfoSize);

	char* buffer = (char*)zmq_msg_data(&msg);
	*(uint16_t*)(buffer) = htons(Message::VERSION);
	buffer += 2;
	*(uint16_t*)(buffer) = htons(Message::PUB_ADDED);
	buffer += 2;
	buffer = writePubInfo(buffer, pub);

	assert(buffer - (char*)zmq_msg_data(&msg) == zmq_msg_size(&msg));

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::sendSubAdded(void* socket, const Subscriber& sub, const PublisherStub& pub, bool hasMore) {
	LOG_INFO("Sending sub added for %s on %s to publisher %s",
	         sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());
	zmq_msg_t msg;
	int subInfoSize = PUB_INFO_SIZE(pub) + SUB_INFO_SIZE(sub) + 2 + 2;
	ZMQ_PREPARE(msg, subInfoSize);

	char* buffer = (char*)zmq_msg_data(&msg);
	*(uint16_t*)(buffer) = htons(Message::VERSION);
	buffer += 2;
	*(uint16_t*)(buffer) = htons(Message::SUBSCRIBE);
	buffer += 2;
	buffer = writePubInfo(buffer, pub);
	buffer = writeSubInfo(buffer, sub);

	assert((size_t)(buffer - (char*)zmq_msg_data(&msg)) == zmq_msg_size(&msg));

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::sendSubRemoved(void* socket, const Subscriber& sub, const PublisherStub& pub, bool hasMore) {
	LOG_INFO("Sending sub removed for %s on %s to publisher %s",
	         sub.getChannelName().c_str(), SHORT_UUID(sub.getUUID()).c_str(), SHORT_UUID(pub.getUUID()).c_str());
	zmq_msg_t msg;
	int subInfoSize = PUB_INFO_SIZE(pub) + SUB_INFO_SIZE(sub) + 2 + 2;
	ZMQ_PREPARE(msg, subInfoSize);

	char* buffer = (char*)zmq_msg_data(&msg);
	*(uint16_t*)(buffer) = htons(Message::VERSION);
	buffer += 2;
	*(uint16_t*)(buffer) = htons(Message::UNSUBSCRIBE);
	buffer += 2;
	buffer = writePubInfo(buffer, pub);
	buffer = writeSubInfo(buffer, sub);

	assert((size_t)(buffer - (char*)zmq_msg_data(&msg)) == zmq_msg_size(&msg));

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

}

void ZeroMQNode::run() {
	char* remoteUUID = NULL;

	int more;
	size_t more_size = sizeof (more);

	//  Initialize poll set - order matters, process lowest frequency first
	zmq_pollitem_t items [] = {
		{ _readOpSocket, 0, ZMQ_POLLIN, 0 }, // one of our members wants to manipulate a socket
		{ _nodeSocket,   0, ZMQ_POLLIN, 0 }, // node meta communication
		{ _pubSocket,    0, ZMQ_POLLIN, 0 }, // read subscriptions
		{ _subSocket,    0, ZMQ_POLLIN, 0 }, // publication requests
	};

	while(isStarted()) {
		zmq_msg_t message;

		int rc = zmq_poll(items, 4, 20);
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
		if (items[2].revents & ZMQ_POLLIN) {
			/**
			 * someone subscribed, process here to avoid
			 * XPUB socket and thread at publisher
			 */
			ScopeLock lock(_mutex);
			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message);
				zmq_msg_recv (&message, _pubSocket, 0);

				char* data = (char*)zmq_msg_data(&message);
				bool subscription = (data[0] == 0x1);
				std::string subChannel(data+1, zmq_msg_size(&message) - 1);

				if (subscription) {
					LOG_DEBUG("Got 0MQ subscription on %s", subChannel.c_str());
					_subscriptions.insert(subChannel);
					processZMQCommSubscriptions(subChannel);
				} else {
					LOG_DEBUG("Got 0MQ unsubscription on %s", subChannel.c_str());
					_subscriptions.erase(subChannel);
					processZMQCommUnsubscriptions(subChannel);
				}

				zmq_getsockopt (_pubSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_close (&message);
				assert(!more); // subscriptions are not multipart
				if (!more)
					break;      //  Last message part
			}
		}

		if (items[3].revents & ZMQ_POLLIN) {
			/**
			 * someone internal publisher wants something published
			 */
			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message);
				zmq_msg_recv (&message, _subSocket, 0);
				zmq_getsockopt (_subSocket, ZMQ_RCVMORE, &more, &more_size);
				zmq_msg_send (&message, _pubSocket, more? ZMQ_SNDMORE: 0);
				zmq_msg_close (&message);
				if (!more)
					break;      //  Last message part
			}
		}

		if (items[1].revents & ZMQ_POLLIN) {
			/**
			 * received some node communication
			 */
			ScopeLock lock(_mutex);

			while (1) {
				//  Process all parts of the message
				zmq_msg_init (&message);
				zmq_msg_recv (&message, _nodeSocket, 0);
				int msgSize = zmq_msg_size(&message);

				zmq_getsockopt (_nodeSocket, ZMQ_RCVMORE, &more, &more_size);

				if (msgSize == 36 && remoteUUID == NULL) {
					// remote id from envelope
					remoteUUID = (char*)malloc(37);
					memcpy(remoteUUID, zmq_msg_data(&message), 36);
					remoteUUID[36] = 0;

					if (_nodes.find(remoteUUID) != _nodes.end())
						_nodes[remoteUUID].getImpl()->setLastSeen(Thread::getTimeStampMs());

				} else if (msgSize >= 2 && remoteUUID != NULL) {
					char* buffer = (char*)zmq_msg_data(&message);
					char* start = buffer;
					(void)start;

					uint16_t version = ntohs(*(short*)(buffer));
					buffer += 2;
					uint16_t type    = ntohs(*(short*)(buffer));
					buffer += 2;

					if (version != Message::VERSION) {
						LOG_ERR("Received message from wrong or unversioned umundo - ignoring");
						zmq_msg_close (&message);
						break;
					}

					switch (type) {
					case Message::DATA:
						break;
					case Message::PUB_ADDED:
					case Message::PUB_REMOVED: {
						// notification of publishers is the first thing a remote node does once it discovers us

						uint16_t port = 0;
						char* channel;
						char* pubUUID;
						buffer = readPubInfo(buffer, port, channel, pubUUID);

						if (buffer - (char*)zmq_msg_data(&message) != msgSize) {
							LOG_ERR("Malformed PUB_ADDED|PUB_REMOVED message received - ignoring");
							break;
						}

						if (type == Message::PUB_ADDED) {
							PublisherStub pubStub(boost::shared_ptr<PublisherStubImpl>(new PublisherStubImpl()));
							pubStub.getImpl()->setChannelName(channel);
							pubStub.getImpl()->setPort(port);
							pubStub.getImpl()->setUUID(pubUUID);
							pubStub.getImpl()->setDomain(remoteUUID);
							processNodeCommPubAdded(remoteUUID, pubStub);
						} else {
							if (_nodes.find(remoteUUID) == _nodes.end()) {
								LOG_ERR("Unknown node removed publisher - ignoring");
								break;
							}
							if (!_nodes[remoteUUID].getPublisher(pubUUID)) {
								LOG_ERR("Unknown node sent unsubscription - ignoring");
								break;
							}
							processNodeCommPubRemoval(remoteUUID, _nodes[remoteUUID].getPublisher(pubUUID));
						}
						break;
					}

					case Message::SUBSCRIBE:
					case Message::UNSUBSCRIBE: {
						// a remote node tells us of a subscription of one of its subscribers

						if (_nodes.find(remoteUUID) == _nodes.end()) {
							LOG_ERR("Got a subscription from an unknown node - this should never happen!");
							break;
						}
						NodeStub nodeStub = _nodes[remoteUUID];

						uint16_t port;
						char* pubChannel;
						char* pubUUID;
						char* subChannel;
						char* subUUID;
						buffer = readPubInfo(buffer, port, pubChannel, pubUUID);
						buffer = readSubInfo(buffer, subChannel, subUUID);

						if (buffer - (char*)zmq_msg_data(&message) != msgSize ||
						        strlen(pubUUID) != 36 ||
						        strlen(subUUID) != 36) {
							LOG_ERR("Malformed SUBSCRIBE|UNSUBSCRIBE message received - ignoring");
							break;
						}

						if (type == Message::SUBSCRIBE) {
							LOG_DEBUG("Got subscription on %s from %s", subChannel, subUUID);

							if (!nodeStub.getSubscriber(subUUID)) {
								SubscriberStub subStub(boost::shared_ptr<SubscriberStubImpl>(new SubscriberStubImpl()));
								subStub.getImpl()->setChannelName(subChannel);
								subStub.getImpl()->setUUID(subUUID);
								nodeStub.addSubscriber(subStub);
							}
							processNodeCommSubscriptions(nodeStub, nodeStub.getSubscriber(subUUID));

						} else {
							LOG_DEBUG("Got unsubscription on %s from %s", subChannel, subUUID);
							if (!nodeStub.getSubscriber(subUUID)) {
								LOG_WARN("Got an unsubscription from an unknown subscriber!");
								break;
							}
							processNodeCommUnsubscriptions(nodeStub, nodeStub.getSubscriber(subUUID));
						}
						break;
					}
					default:
						LOG_WARN("Received unknown message type");
					}
				}

				zmq_msg_close (&message);
				if (!more) {
					free(remoteUUID);
					remoteUUID = NULL;
					break;      //  Last message part
				}
			}
		}
	}
}

void ZeroMQNode::processNodeCommPubAdded(const std::string& nodeUUID, const PublisherStub& pubStub) {
	if (_nodes.find(nodeUUID) != _nodes.end()) {
		// we already know the node
		confirmPubAdded(_nodes[nodeUUID], pubStub);
	} else {
		_pendingRemotePubs[nodeUUID][pubStub.getUUID()] = pubStub;
	}
}

void ZeroMQNode::confirmPubAdded(NodeStub& nodeStub, const PublisherStub& pubStub) {
	pubStub.getImpl()->setHost(nodeStub.getHost());
	pubStub.getImpl()->setTransport(nodeStub.getTransport());
	pubStub.getImpl()->setIP(nodeStub.getIP());
	pubStub.getImpl()->setRemote(nodeStub.isRemote());
	pubStub.getImpl()->setInProcess(nodeStub.isInProcess());
	nodeStub.addPublisher(pubStub);

	// see if we have a local subscriber interested in the publisher's channel
	std::map<std::string, Subscriber>::iterator subIter;
	for (subIter = _subs.begin(); subIter != _subs.end(); subIter++) {
		if(pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
			subIter->second.added(pubStub);
			sendSubAdded(_sockets[nodeStub.getUUID()], subIter->second, pubStub, false);
		}
	}
	assert(validateState());

}

void ZeroMQNode::processNodeCommPubRemoval(const std::string& nodeUUID, const PublisherStub& pubStub) {
	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		if(pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
			subIter->second.removed(pubStub);
		}
		subIter++;
	}
	assert(validateState());
}

void ZeroMQNode::processZMQCommUnsubscriptions(const std::string& channel) {
	// we might not get unsubscriptions with vanishing nodes with embedded mDNS servers, so we process these here
	if (UUID::isUUID(channel.substr(1))) {
		string subUUID(channel.substr(1));
		// find us the node of this subscriber
		map<string, NodeStub>::iterator nodeIter = _nodes.begin();
		while(nodeIter != _nodes.end()) {
			SubscriberStub subStub = nodeIter->second.getSubscriber(subUUID);
			if (subStub) {
				processNodeCommUnsubscriptions(nodeIter->second, subStub);
			}
			nodeIter++;
		}
	}
	
}

void ZeroMQNode::processZMQCommSubscriptions(const std::string& channel) {
	// we have a subscription from zeromq, try to confirm pending subscriptions from node communication
	std::multimap<std::string, std::pair<long, std::pair<NodeStub, SubscriberStub> > >::iterator subIter = _pendingSubscriptions.find(channel);
	while(subIter != _pendingSubscriptions.end()) {
		std::pair<NodeStub, SubscriberStub> subPair = subIter->second.second;
		if (confirmSubscription(subPair.first, subPair.second))
			return;
		subIter++;
	}
}

void ZeroMQNode::processNodeCommSubscriptions(const NodeStub& nodeStub, const SubscriberStub& subStub) {
	subStub.getImpl()->setLastSeen(Thread::getTimeStampMs());
	std::pair<NodeStub, SubscriberStub> subPair = std::make_pair(nodeStub, subStub);
	long now = Thread::getTimeStampMs();
	std::pair<long, std::pair<NodeStub, SubscriberStub> > timedSubPair = std::make_pair(now, subPair);

	std::string subId = std::string("~" + subStub.getUUID());
	_pendingSubscriptions.insert(std::make_pair(subId, timedSubPair));

	confirmSubscription(nodeStub, subStub);

}

void ZeroMQNode::processNodeCommUnsubscriptions(NodeStub& nodeStub, const SubscriberStub& subStub) {
	subStub.getImpl()->setLastSeen(Thread::getTimeStampMs());

	if (_confirmedSubscribers.count(subStub.getUUID()) > 0) {
		LOG_INFO("Removing confirmed subscriber: %s", subStub.getUUID().c_str());
		_confirmedSubscribers.erase(subStub.getUUID());
	} else {
    return; // zeromq might already have removed the subscriber
//		LOG_WARN("Trying to remove unconfirmed subscriber: %s", subStub.getUUID().c_str());
	}

#if 0
	// we never receive 0mq unsubscriptions for these due to the buggy zmq_disconnect
	std::multiset<std::string>::iterator found;
	found = _subscriptions.find("~" + subStub.getUUID());
	if (found != _subscriptions.end()) _subscriptions.erase(found);

	found = _subscriptions.find(subStub.getChannelName());
	if (found != _subscriptions.end()) _subscriptions.erase(found);
#endif

	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while(pubIter != _pubs.end()) {
		if(pubIter->second.getChannelName().substr(0, subStub.getChannelName().size()) == subStub.getChannelName()) {
			pubIter->second.removedSubscriber(nodeStub.getUUID(), subStub.getUUID());
		}
		pubIter++;
	}

	if (_nodes.find(nodeStub.getUUID()) != _nodes.end()) {
		_nodes[nodeStub.getUUID()].removeSubscriber(subStub);
	}

	assert(validateState());
}

bool ZeroMQNode::confirmSubscription(const NodeStub& nodeStub, const SubscriberStub& subStub) {
	// see whether we have all the subscribers subscriptions
	std::string subId = std::string("~" + subStub.getUUID());

	int receivedSubs;
	int confirmedSubs;
	receivedSubs = _subscriptions.count(subId);
	confirmedSubs = _confirmedSubscribers.count(subStub.getUUID());
	if (receivedSubs <= confirmedSubs)
		return false;

	// we received all zeromq subscriptions for this node, notify publishers
	LOG_INFO("Confirming subscriber: %s", subStub.getUUID().c_str());
	_confirmedSubscribers.insert(subStub.getUUID());

	// remove from pending subscriptions
	std::multimap<std::string, std::pair<long, std::pair<NodeStub, SubscriberStub> > >::iterator subIter;
	subIter = _pendingSubscriptions.find(subId);
	while(subIter != _pendingSubscriptions.end()) {
		std::pair<NodeStub, SubscriberStub> subPair = subIter->second.second;
		if (subPair.first == nodeStub && subPair.second == subStub)
			_pendingSubscriptions.erase(subIter++);
		else subIter++;
	}

	nodeStub.getImpl()->addSubscriber(subStub);

	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while(pubIter != _pubs.end()) {
		if(pubIter->second.getChannelName().substr(0, subStub.getChannelName().size()) == subStub.getChannelName()) {
			pubIter->second.addedSubscriber(nodeStub.getUUID(), subStub.getUUID());
		}
		pubIter++;
	}

	assert(validateState());
	return true;
}

/**
 * Write channel\0uuid\0port into the given byte array
 */
char* ZeroMQNode::writePubInfo(char* buffer, const PublisherStub& pub) {
	string channel = pub.getChannelName();
	string uuid = pub.getUUID(); // we share a publisher socket in the node, do not publish hidden pub uuid
	uint16_t port = _pubPort; //pub.getPort();

	assert(uuid.length() == 36);

	char* start = buffer;
	(void)start; // surpress unused warning wiithout assert

	memcpy(buffer, channel.c_str(), channel.length() + 1);
	buffer += channel.length() + 1;
	memcpy(buffer, uuid.c_str(), uuid.length() + 1);
	buffer += uuid.length() + 1;
	*(uint16_t*)buffer = htons(port);
	buffer += 2;

	assert(buffer - start == PUB_INFO_SIZE(pub));
	return buffer;
}

char* ZeroMQNode::readPubInfo(char* buffer, uint16_t& port, char*& channelName, char*& uuid) {
	char* start = buffer;
	(void)start; // surpress unused warning without assert

	channelName = buffer;
	buffer += strlen(buffer) + 1;
	uuid = buffer;
	buffer += strlen(buffer) + 1;
	port = ntohs(*(short*)(buffer));
	buffer += 2;

	assert(buffer - start == (int)strlen(channelName) + 1 + (int)strlen(uuid) + 1 + 2);
	return buffer;
}

char* ZeroMQNode::writeSubInfo(char* buffer, const Subscriber& sub) {
	string channel = sub.getChannelName();
	string uuid = sub.getUUID(); // we share a publisher socket in the node, do not publish hidden pub uuid

	assert(uuid.length() == 36);

	char* start = buffer;
	(void)start; // surpress unused warning without assert

	memcpy(buffer, channel.c_str(), channel.length() + 1);
	buffer += channel.length() + 1;
	memcpy(buffer, uuid.c_str(), uuid.length() + 1);
	buffer += uuid.length() + 1;

	assert(buffer - start == SUB_INFO_SIZE(sub));
	return buffer;
}

char* ZeroMQNode::readSubInfo(char* buffer, char*& channelName, char*& uuid) {
	char* start = buffer;
	(void)start; // surpress unused warning without assert

	channelName = buffer;
	buffer += strlen(channelName) + 1;
	uuid = buffer;
	buffer += strlen(uuid) + 1;

	assert(buffer - start == (int)strlen(channelName) + 1 + (int)strlen(uuid) + 1);
	return buffer;
}

bool ZeroMQNode::validateState() {
	ScopeLock lock(_mutex);
	map<string, NodeStub>::iterator nodeIter = _nodes.begin();
	map<string, void*>::iterator nodeSockIter = _sockets.begin();
#if 0
	std::multiset<std::string>::iterator zmqSubIter = _subscriptions.begin();
	std::set<std::string>::iterator confirmedSubIter = _confirmedSubscribers.begin();
	std::multimap<std::string, std::pair<long, std::pair<umundo::NodeStub, umundo::SubscriberStub> > >::iterator pendingSubIter = _pendingSubscriptions.begin();
	std::map<std::string, std::map<std::string, umundo::PublisherStub> >::iterator pendingPubIter = _pendingRemotePubs.begin();
	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
#endif
	// gather subIds and channels from nodes
	std::map<std::string, PublisherStub> remotePubs;
	std::map<std::string, SubscriberStub> remoteSubs;

	// make sure we have sockets to all nodes and vice versa
	nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		std::map<std::string, PublisherStub> nodePublishers = nodeIter->second.getPublishers();
		std::map<std::string, SubscriberStub> nodeSubscribers = nodeIter->second.getSubscribers();
		remotePubs.insert(nodePublishers.begin(), nodePublishers.end());
		remoteSubs.insert(nodeSubscribers.begin(), nodeSubscribers.end());

		assert(_sockets.find(nodeIter->first) != _sockets.end());
		nodeIter++;
	}

	nodeSockIter = _sockets.begin();
	while(nodeSockIter != _sockets.end()) {
		assert(_nodes.find(nodeSockIter->first) != _nodes.end());
		nodeSockIter++;
	}

	std::multiset<std::string> staleSubscriptions = _subscriptions;
	// every subscriber we actually added to node had to be confirmed - check
	// also remove subscribers ids and channels from 0mq subscriptions and see what's left
	std::map<std::string, SubscriberStub>::iterator remoteSubIter = remoteSubs.begin();

//  std::cout << "ZMQ Subs " << _subscriptions.size() << std::endl;
	for(std::multiset<std::string>::iterator staleSubsIter = staleSubscriptions.begin(); staleSubsIter != staleSubscriptions.end(); staleSubsIter++) {
//    std::cout << "\t" << *staleSubsIter << std::endl;
	}

//  std::cout << "Confirmed Subs " << _confirmedSubscribers.size() << std::endl;
	for(std::set<std::string>::iterator confirmSubsIter = _confirmedSubscribers.begin(); confirmSubsIter != _confirmedSubscribers.end(); confirmSubsIter++) {
//    std::cout << "\t" << *confirmSubsIter << std::endl;
	}

//  std::cout << "uMundo Subs " << remotePubs.size() << std::endl;
	while(remoteSubIter != remoteSubs.end()) {
//    std::cout << "\t" << remoteSubIter->first << std::endl;
//    std::cout << "\t" << remoteSubIter->second.getChannelName() << std::endl;

		assert(_confirmedSubscribers.find(remoteSubIter->first) != _confirmedSubscribers.end());
//    assert(staleSubscriptions.count(remoteSubIter->second.getChannelName()) > 0);

		std::multiset<std::string>::iterator staleFound;

		staleFound = staleSubscriptions.find("~" + remoteSubIter->first);
		if (staleFound != staleSubscriptions.end()) staleSubscriptions.erase(staleFound);

		staleFound = staleSubscriptions.find(remoteSubIter->second.getChannelName());
		if (staleFound != staleSubscriptions.end()) staleSubscriptions.erase(staleFound);

		remoteSubIter++;
	}

//  std::cout << "Stale Subs " << staleSubscriptions.size() << std::endl;
	for(std::multiset<std::string>::iterator staleSubsIter = staleSubscriptions.begin(); staleSubsIter != staleSubscriptions.end(); staleSubsIter++) {
//    std::cout << "\t" << *staleSubsIter << std::endl;
	}

	assert(staleSubscriptions.size() < 20); // only for testing

	return true;
}

}
