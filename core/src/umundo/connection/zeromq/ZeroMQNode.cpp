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
		(_zmqContext = zmq_ctx_new()) || UM_LOG_ERR("zmq_init: %s",zmq_strerror(errno));
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

	(_pubSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_XPUB))    || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_subSocket     = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_SUB))     || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_nodeSocket    = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_ROUTER))  || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_readOpSocket  = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
	(_writeOpSocket = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_PAIR))    || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

	std::string readOpId("inproc://um.node.readop." + _uuid);
	zmq_bind(_readOpSocket, readOpId.c_str())  && UM_LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
	zmq_connect(_writeOpSocket, readOpId.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", readOpId.c_str(), zmq_strerror(errno));

	int sndhwm = NET_ZEROMQ_SND_HWM;
	int rcvhwm = NET_ZEROMQ_RCV_HWM;
	int vbsSub = 1;

	std::string pubId("um.pub." + _uuid);
//	zmq_setsockopt(_pubSocket, ZMQ_IDENTITY, pubId.c_str(), pubId.length()) && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_pubSocket, ZMQ_SNDHWM, &sndhwm, sizeof(sndhwm))               && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_pubSocket, ZMQ_XPUB_VERBOSE, &vbsSub, sizeof(vbsSub))   && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // receive all subscriptions

	zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &rcvhwm, sizeof(rcvhwm)) && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
	zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, "", 0)          && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // subscribe to every internal publisher

	std::string nodeId("um.node." + _uuid);
//	zmq_setsockopt(_nodeSocket, ZMQ_IDENTITY, nodeId.c_str(), nodeId.length()) && UM_LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));

	_port = bindToFreePort(_nodeSocket, "tcp", "*");
	zmq_bind(_nodeSocket, std::string("inproc://" + nodeId).c_str()) && UM_LOG_WARN("zmq_bind: %s", zmq_strerror(errno));
//  zmq_bind(_nodeSocket, std::string("ipc://" + nodeId).c_str())    && UM_LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + nodeId).c_str(),  zmq_strerror(errno));

	_pubPort = bindToFreePort(_pubSocket, "tcp", "*");
	zmq_bind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && UM_LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
//  zmq_bind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && UM_LOG_WARN("zmq_bind: %s %s", std::string("ipc://" + pubId).c_str(), zmq_strerror(errno))

	UM_LOG_INFO("Node %s listening on port %d", SHORT_UUID(_uuid).c_str(), _port);
	UM_LOG_INFO("Publisher %s listening on port %d", SHORT_UUID(_uuid).c_str(), _pubPort);

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
		zmq_close(sockIter->second) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	}

	join();
	zmq_close(_pubSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_subSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_nodeSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_readOpSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
	zmq_close(_writeOpSocket) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));
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
			UM_LOG_WARN("zmq_bind at %s: %s", ss.str().c_str(), zmq_strerror(errno));
			Thread::sleepMs(100);
		}
	}

	return port;
}

void ZeroMQNode::addSubscriber(Subscriber& sub) {
	ScopeLock lock(_mutex);
	if (_subs.find(sub.getUUID()) == _subs.end()) {
		_subs[sub.getUUID()] = sub;

		UM_LOG_INFO("%s added subscriber %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

		std::map<std::string, NodeStub>::iterator nodeIter = _nodes.begin();
		while (nodeIter != _nodes.end()) {
			assert(_sockets.find(nodeIter->first) != _sockets.end());
			void* nodeSocket = _sockets[nodeIter->first];
			assert(nodeSocket != NULL);

			std::map<std::string, PublisherStub> pubs = nodeIter->second.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
			// iterate all remote publishers
			while (pubIter != pubs.end()) {
				if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
					sub.added(pubIter->second);
					zmq_msg_t uuidMsg;
					ZMQ_SEND_IDENTITY(uuidMsg, _uuid, nodeSocket);
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

		UM_LOG_INFO("%s removed subscriber %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(sub.getUUID()).c_str(), sub.getChannelName().c_str());

		std::map<std::string, NodeStub>::iterator nodeIter = _nodes.begin();
		while (nodeIter != _nodes.end()) {
			assert(_sockets.find(nodeIter->first) != _sockets.end());
			void* nodeSocket = _sockets[nodeIter->first];

			std::map<std::string, PublisherStub> pubs = nodeIter->second.getPublishers();
			std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
			// iterate all remote publishers
			while (pubIter != pubs.end()) {
				if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
					zmq_msg_t uuidMsg;
					ZMQ_SEND_IDENTITY(uuidMsg, _uuid, nodeSocket);
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
		ZMQ_INTERNAL_SEND("connectPub", internalPubId.c_str());

		UM_LOG_INFO("%s added publisher %s on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

		_pubs[pub.getUUID()] = pub;
		std::map<std::string, void*>::iterator socketIter = _sockets.begin();
		// notify all other nodes
		while (socketIter != _sockets.end()) {
			zmq_msg_t uuidMsg;
			ZMQ_SEND_IDENTITY(uuidMsg, _uuid, socketIter->second);
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
		ZMQ_INTERNAL_SEND("disconnectPub", internalPubId.c_str());

		UM_LOG_INFO("%s removed publisher %d on %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(pub.getUUID()).c_str(), pub.getChannelName().c_str());

		_pubs.erase(pub.getUUID());
		std::map<std::string, void*>::iterator socketIter = _sockets.begin();
		// notify all other nodes
		while (socketIter != _sockets.end()) {
			zmq_msg_t uuidMsg;
			ZMQ_SEND_IDENTITY(uuidMsg, _uuid, socketIter->second);
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

	/**
	 * Nodes registered via embedded mDNS might not have unregistered and someone else might
	 * already have taken their place. mDNS will report these from cache as well so we might
	 * get multiple nodes with the same address.
	 */

	if (_nodes.find(node.getUUID()) != _nodes.end()) {
		UM_LOG_WARN("Not adding already known node");
		_nodes[node.getUUID()].getImpl()->setLastSeen(Thread::getTimeStampMs());
		return;
	}

	UM_LOG_INFO("Discovery reports node %s at %s://%s:%d",
	            SHORT_UUID(node.getUUID()).c_str(),
	            node.getTransport().c_str(),
	            node.getIP().c_str(),
	            node.getPort());

	// establish connection
	std::stringstream ss;
	if (node.isInProcess()) {
		ss << "inproc://um.node." << node.getUUID();  // same process, use inproc communication
	} else if (!node.isRemote() && false) {           // disabled for now as it creates an actual file on unices
		ss << "ipc://um.node." << node.getUUID();     // same host, use inter-process communication
	} else {
		ss << node.getTransport() << "://" << node.getIP() << ":" << node.getPort();  // remote node, use network
	}

	// connect a socket to the remote node port
	void* client;
	(client = zmq_socket(ZeroMQNode::getZeroMQContext(), ZMQ_DEALER)) || UM_LOG_ERR("zmq_socket: %s",zmq_strerror(errno));

	// give the socket an id for zeroMQ routing
//	zmq_setsockopt(client, ZMQ_IDENTITY, _uuid.c_str(), _uuid.length()) && UM_LOG_ERR("zmq_setsockopt: %s",zmq_strerror(errno));
	zmq_connect(client, ss.str().c_str()) && UM_LOG_ERR("%s zmq_connect %s: %s", SHORT_UUID(_uuid).c_str(), ss.str().c_str(), zmq_strerror(errno));

	assert(client);
	// remember node stub as pending and open socket
	_pendingNodes[node.getUUID()].nodeStub = node;
	_pendingNodes[node.getUUID()].lastSeen = Thread::getTimeStampMs();
	_sockets[node.getUUID()] = client;

	// send local publishers to remote node - this might be called multiple times for the same endpoint because of the embedded mDNS issues
	if (_pubs.size() > 0) {
		UM_LOG_INFO("sending %d publishers to %s", _pubs.size(), ss.str().c_str());
		std::map<std::string, Publisher>::iterator pubIter;
		int hasMore = _pubs.size();
		zmq_msg_t uuidMsg;
		ZMQ_SEND_IDENTITY(uuidMsg, _uuid, client);
		for (pubIter = _pubs.begin(); pubIter != _pubs.end(); pubIter++) {
			sendPubAdded(client, pubIter->second, (--hasMore > 0));
		}
		assert(hasMore == 0);
	} else {
		// send something to confirm node per node socket
		UM_LOG_INFO("No publishers to send - sending KEEP_ALIVE to %s", ss.str().c_str());
		zmq_msg_t uuidMsg;
		ZMQ_SEND_IDENTITY(uuidMsg, _uuid, client);
		zmq_msg_t msg;
		ZMQ_PREPARE(msg, 4);
		char* buffer = (char*)zmq_msg_data(&msg);
		*(uint16_t*)(buffer) = htons(Message::VERSION);
		buffer += 2;
		*(uint16_t*)(buffer) = htons(Message::KEEP_ALIVE);
		buffer += 2;
		zmq_sendmsg(client, &msg, 0) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
		zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
	}

	// confirm node
	processConfirmedNode(_pendingNodes[node.getUUID()]);

	assert(validateState());
}

void ZeroMQNode::removed(NodeStub node) {
	// is this us?
	if (node.getUUID() == _uuid)
		return;
	ScopeLock lock(_mutex);

	UM_LOG_INFO("Lost node %s", SHORT_UUID(node.getUUID()).c_str());

	if (_nodes.find(node.getUUID()) == _nodes.end()) {
		UM_LOG_INFO("Not removing unknown node");
		if (_sockets.find(node.getUUID()) != _sockets.end()) {
			zmq_close(_sockets[node.getUUID()]);
			_sockets.erase(node.getUUID());               // delete socket
		}
		return;
	}

	std::map<std::string, PublisherStub> pubs = _nodes[node.getUUID()].getPublishers();
	std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
	while (pubIter != pubs.end()) {
		processRemotePubRemoval(node, pubIter->second);
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
		UM_LOG_WARN("removed: client was never added: %s", SHORT_UUID(node.getUUID()).c_str());
		return;
	}

	assert(_sockets.find(node.getUUID()) != _sockets.end());
	zmq_close(_sockets[node.getUUID()]) && UM_LOG_WARN("zmq_close: %s",zmq_strerror(errno));

	_sockets.erase(node.getUUID());               // delete socket
	_nodes.erase(node.getUUID());
	_pendingPublishers.erase(node.getUUID());
	_pendingSubscriptions.erase(node.getUUID());

//	assert(_sockets.size() == _nodes.size() + _pendingNodes.size());

	UM_LOG_INFO("%s removed %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(node.getUUID()).c_str());
	assert(validateState());

}

void ZeroMQNode::changed(NodeStub node) {
}

void ZeroMQNode::processConfirmedNode(const PendingNode& node) {
	if (!node.isConfirmed())
		return;

	// see if we had a zombie at the same endpoint and remove it
	_nodes_t::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		NodeStub nodeStub = nodeIter->second;
		nodeIter++;
		if (
		    nodeStub.getIP() == node.nodeStub.getIP() &&
		    nodeStub.getPort() == node.nodeStub.getPort() &&
		    nodeStub.getTransport() == node.nodeStub.getTransport()
		) {
			removed(nodeStub);
		}
	}

	// move form pending to confirmed
	_nodes[node.nodeStub.getUUID()] = node.nodeStub;

	UM_LOG_INFO("%s confirmed node discovery of %s at %s://%s:%d",
	            SHORT_UUID(_uuid).c_str(),
	            SHORT_UUID(node.nodeStub.getUUID()).c_str(),
	            node.nodeStub.getTransport().c_str(),
	            node.nodeStub.getIP().c_str(),
	            node.nodeStub.getPort());

	// check whether we can also confirm its publishers
	std::pair<_pendingPubs_t::iterator, _pendingPubs_t::iterator> pubRange = _pendingPublishers.equal_range(node.nodeStub.getUUID());
	for (_pendingPubs_t::iterator it = pubRange.first; it != pubRange.second; ++it) {
		assert(it->second.pubStub);
		it->second.nodeStub = node.nodeStub;
		it->second.pubStub.getImpl()->setInProcess(node.nodeStub.isInProcess());
		it->second.pubStub.getImpl()->setRemote(node.nodeStub.isRemote());
		it->second.pubStub.getImpl()->setIP(node.nodeStub.getIP());
		it->second.pubStub.getImpl()->setTransport(node.nodeStub.getTransport());
		processConfirmedPublisher(it->second);
	}

	// check subscribers as well
	std::pair<_pendingSubs_t::iterator, _pendingSubs_t::iterator> subRange = _pendingSubscriptions.equal_range(node.nodeStub.getUUID());
	for (_pendingSubs_t::iterator it = subRange.first; it != subRange.second; ++it) {
		assert(it->second.subStub);
		it->second.nodeStub = node.nodeStub;
		it->second.subStub.getImpl()->setInProcess(node.nodeStub.isInProcess());
		it->second.subStub.getImpl()->setRemote(node.nodeStub.isRemote());
		it->second.subStub.getImpl()->setIP(node.nodeStub.getIP());
		it->second.subStub.getImpl()->setTransport(node.nodeStub.getTransport());
		processConfirmedSubscription(it->second);
	}

	_pendingNodes.erase(node.nodeStub.getUUID());

}

void ZeroMQNode::processConfirmedSubscription(const PendingSubscriber& sub) {
	if (!sub.isConfirmed())
		return;

	assert(sub.nodeStub);
	if (_nodes.find(sub.nodeStub.getUUID()) == _nodes.end())
		return;

	UM_LOG_INFO("%s confirmed remote subscriber of %s at %s",
	            SHORT_UUID(_uuid).c_str(),
	            SHORT_UUID(sub.subStub.getChannelName()).c_str(),
	            sub.nodeStub.getIP().c_str());

	sub.nodeStub.getImpl()->addSubscriber(sub.subStub);

	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while(pubIter != _pubs.end()) {
		if(pubIter->second.getChannelName().substr(0, sub.subStub.getChannelName().size()) == sub.subStub.getChannelName()) {
			pubIter->second.addedSubscriber(sub.nodeStub.getUUID(), sub.subStub.getUUID());
		}
		pubIter++;
	}

	_pendingSubscriptions.erase(sub.subStub.getUUID());
}

void ZeroMQNode::processConfirmedPublisher(const PendingPublisher& pub) {
	if (!pub.isConfirmed())
		return;

	assert(pub.nodeStub);
	if (_nodes.find(pub.nodeStub.getUUID()) == _nodes.end())
		return;

	UM_LOG_INFO("%s confirmed remote publisher of %s at %s://%s:%d",
	            SHORT_UUID(_uuid).c_str(),
	            SHORT_UUID(pub.pubStub.getChannelName()).c_str(),
	            pub.pubStub.getTransport().c_str(),
	            pub.pubStub.getIP().c_str(),
	            pub.pubStub.getPort());

	pub.nodeStub.getImpl()->addPublisher(pub.pubStub);

	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		if(pub.pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
			subIter->second.added(pub.pubStub);
			assert(_sockets.find(pub.nodeStub.getUUID()) != _sockets.end());
			zmq_msg_t uuidMsg;
			ZMQ_SEND_IDENTITY(uuidMsg, _uuid, _sockets[pub.nodeStub.getUUID()]);
			sendSubAdded(_sockets[pub.nodeStub.getUUID()], subIter->second, pub.pubStub, false);
		}
		subIter++;
	}

	_pendingPublishers.erase(pub.pubStub.getUUID());
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

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
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

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::sendSubAdded(void* socket, const Subscriber& sub, const PublisherStub& pub, bool hasMore) {
	UM_LOG_INFO("Sending sub added for %s on %s to publisher %s",
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

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
}

void ZeroMQNode::sendSubRemoved(void* socket, const Subscriber& sub, const PublisherStub& pub, bool hasMore) {
	UM_LOG_INFO("Sending sub removed for %s on %s to publisher %s",
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

	zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
	zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

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
			UM_LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
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
					UM_LOG_INFO("Got 0MQ subscription on %s", subChannel.c_str());

					// see if we already have a pending subscription and zmq confirm it
					if (UUID::isUUID(subChannel.substr(1))) {
						// only insert unique uuid subscriptions as ZMQ_XPUB_VERBOSE does not work for unsubscriptions
						_subscriptions.insert(subChannel);
						_pendingSubs_t::iterator subIter = _pendingSubscriptions.begin();
						while(subIter != _pendingSubscriptions.end()) {
							if (subIter->second.subStub.getUUID() == subChannel.substr(1)) {
								subIter->second.zmqConfirmed = true;
								subIter->second.lastSeen = Thread::getTimeStampMs();
								processConfirmedSubscription(subIter->second);
							}
							subIter++;
						}
					}
				} else {
					UM_LOG_INFO("Got 0MQ unsubscription on %s", subChannel.c_str());
					if (UUID::isUUID(subChannel.substr(1))) {
						_subscriptions.erase(subChannel);
						map<string, NodeStub>::iterator nodeIter = _nodes.begin();
						while(nodeIter != _nodes.end()) {
							SubscriberStub subStub = nodeIter->second.getSubscriber(subChannel.substr(1));
							if (subStub) {
								processRemoteSubRemoval(nodeIter->second, subStub);
							}
							nodeIter++;
						}
					}
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
					// remote id from ZMQ router envelope
					remoteUUID = (char*)malloc(37);
					memcpy(remoteUUID, zmq_msg_data(&message), 36);
					remoteUUID[36] = 0;
					UM_LOG_INFO("%s received node message from %s", SHORT_UUID(string(_uuid)).c_str(), SHORT_UUID(string(remoteUUID)).c_str());

					// make a note of a pending node
					if (_nodes.find(remoteUUID) == _nodes.end()) {
						_pendingNodes[remoteUUID].zmqConfirmed = true;
						processConfirmedNode(_pendingNodes[remoteUUID]);
					}

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
						UM_LOG_ERR("Received message from wrong or unversioned umundo - ignoring");
						zmq_msg_close (&message);
						break;
					} else {
						UM_LOG_INFO("%s received %s message from %s", SHORT_UUID(_uuid).c_str(), Message::typeToString(type), SHORT_UUID(string(remoteUUID)).c_str());
					}

					switch (type) {
					case Message::DATA:
					case Message::KEEP_ALIVE:
						// do nothing for now
						break;
					case Message::PUB_ADDED:
					case Message::PUB_REMOVED: {
						// notification of publishers is the first thing a remote node does once it discovers us

						uint16_t port = 0;
						char* channel;
						char* pubUUID;
						buffer = readPubInfo(buffer, port, channel, pubUUID);

						if (buffer - (char*)zmq_msg_data(&message) != msgSize) {
							UM_LOG_ERR("Malformed PUB_ADDED|PUB_REMOVED message received - ignoring");
							break;
						}

						if (type == Message::PUB_ADDED) {
							UM_LOG_INFO("Received publisher for %s from %s", channel, SHORT_UUID(string(remoteUUID)).c_str());

							std::pair<_pendingPubs_t::iterator, _pendingPubs_t::iterator> pubRange = _pendingPublishers.equal_range(remoteUUID);
							for (_pendingPubs_t::iterator it = pubRange.first; it != pubRange.second; ++it) {
								if (it->second.pubStub.getUUID() == string(pubUUID)) {
									UM_LOG_INFO("Publisher already pending");
									break;
								}
							}

							PendingPublisher pub;
							pub.pubStub = PublisherStub(boost::shared_ptr<PublisherStubImpl>(new PublisherStubImpl()));
							pub.pubStub.getImpl()->setChannelName(channel);
							pub.pubStub.getImpl()->setPort(port);
							pub.pubStub.getImpl()->setUUID(pubUUID);
							pub.pubStub.getImpl()->setDomain(remoteUUID);

							_pendingPublishers.insert(std::make_pair(remoteUUID, pub));
							if (_nodes.find(remoteUUID) != _nodes.end()) {
								assert(pub.pubStub);
								pub.nodeStub = _nodes[remoteUUID];
								pub.pubStub.getImpl()->setInProcess(pub.nodeStub.isInProcess());
								pub.pubStub.getImpl()->setRemote(pub.nodeStub.isRemote());
								pub.pubStub.getImpl()->setIP(pub.nodeStub.getIP());
								pub.pubStub.getImpl()->setTransport(pub.nodeStub.getTransport());
							}

							processConfirmedPublisher(pub);

						} else {

							UM_LOG_INFO("Lost publisher for %s from %s", channel, SHORT_UUID(string(remoteUUID)).c_str());
							// we remove per node communication and zeromq, one will not find the publisher or node even
							if (_nodes.find(remoteUUID) == _nodes.end()) {
								UM_LOG_DEBUG("Unknown node removed publisher - ignoring");
								break;
							}
							if (!_nodes[remoteUUID].getPublisher(pubUUID)) {
								UM_LOG_DEBUG("Unknown node sent unsubscription - ignoring");
								break;
							}
							processRemotePubRemoval(_nodes[remoteUUID], _nodes[remoteUUID].getPublisher(pubUUID));
						}
						break;
					}

					case Message::SUBSCRIBE:
					case Message::UNSUBSCRIBE: {
						// a remote node tells us of a subscription of one of its subscribers
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
							UM_LOG_ERR("Malformed SUBSCRIBE|UNSUBSCRIBE message received - ignoring");
							break;
						}

						if (type == Message::SUBSCRIBE) {
							UM_LOG_DEBUG("Got subscription on %s from %s", subChannel, subUUID);

							PendingSubscriber sub;
							sub.subStub = SubscriberStub(boost::shared_ptr<SubscriberStubImpl>(new SubscriberStubImpl()));
							sub.subStub.getImpl()->setChannelName(subChannel);
							sub.subStub.getImpl()->setUUID(subUUID);

							if (_subscriptions.find(string("~" + string(subUUID))) != _subscriptions.end())
								sub.zmqConfirmed = true;

							if (_nodes.find(remoteUUID) != _nodes.end())
								sub.nodeStub = _nodes[remoteUUID];

							_pendingSubscriptions.insert(std::make_pair(remoteUUID, sub));

							processConfirmedSubscription(sub);

						} else {
							UM_LOG_DEBUG("Got unsubscription on %s from %s", subChannel, subUUID);
							NodeStub nodeStub;
							if (_nodes.find(remoteUUID) != _nodes.end())
								nodeStub = _nodes[remoteUUID];

							if (!nodeStub || !nodeStub.getSubscriber(subUUID)) {
								UM_LOG_DEBUG("Got an unsubscription from an unknown subscriber!");
								break;
							}
							processRemoteSubRemoval(nodeStub, nodeStub.getSubscriber(subUUID));
						}
						break;
					}
					default:
						UM_LOG_WARN("Received unknown message type");
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

void ZeroMQNode::processRemotePubRemoval(const NodeStub& nodeStub, const PublisherStub& pubStub) {
	std::map<std::string, Subscriber>::iterator subIter = _subs.begin();
	while(subIter != _subs.end()) {
		if(pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
			subIter->second.removed(pubStub);
		}
		subIter++;
	}
	nodeStub.getImpl()->removePublisher(pubStub);
	assert(validateState());
}

void ZeroMQNode::processRemoteSubRemoval(const NodeStub& nodeStub, const SubscriberStub& subStub) {
	if (!nodeStub.getSubscriber(subStub.getUUID()) == subStub)
		return;

	std::map<std::string, Publisher>::iterator pubIter = _pubs.begin();
	while(pubIter != _pubs.end()) {
		if(pubIter->second.getChannelName().substr(0, subStub.getChannelName().size()) == subStub.getChannelName()) {
			pubIter->second.removedSubscriber(nodeStub.getUUID(), subStub.getUUID());
		}
		pubIter++;
	}
	nodeStub.getImpl()->removeSubscriber(subStub);
	assert(validateState());

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
		assert(_nodes.find(nodeSockIter->first) != _nodes.end() ||
		       _pendingNodes.find(nodeSockIter->first) != _pendingNodes.end());
		nodeSockIter++;
	}

	std::multiset<std::string> staleSubscriptions = _subscriptions;

	// every subscriber we actually added to node had to be confirmed - check
	// also remove subscribers ids and channels from 0mq subscriptions and see what's left
	std::map<std::string, SubscriberStub>::iterator remoteSubIter = remoteSubs.begin();

	while(remoteSubIter != remoteSubs.end()) {
		std::multiset<std::string>::iterator staleFound;

		staleFound = staleSubscriptions.find("~" + remoteSubIter->first);
		if (staleFound != staleSubscriptions.end()) staleSubscriptions.erase(staleFound);

		staleFound = staleSubscriptions.find(remoteSubIter->second.getChannelName());
		if (staleFound != staleSubscriptions.end()) staleSubscriptions.erase(staleFound);

		remoteSubIter++;
	}

//  std::cout << "Stale Subs at " << SHORT_UUID(_uuid) << ":" << staleSubscriptions.size() << std::endl;
//	for(std::multiset<std::string>::iterator staleSubsIter = staleSubscriptions.begin(); staleSubsIter != staleSubscriptions.end(); staleSubsIter++) {
//    std::cout << "\t" << *staleSubsIter << std::endl;
//	}

//	assert(staleSubscriptions.size() < 2); // only for testing

	return true;
}

}
