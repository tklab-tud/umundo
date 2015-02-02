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

#include "umundo/core/connection/Node.h"

#include "umundo/core/Factory.h"
#include "umundo/core/UUID.h"
#include "umundo/core/discovery/Discovery.h"
#include "umundo/core/connection/Subscriber.h"
#include "umundo/core/connection/Publisher.h"

namespace umundo {

int NodeImpl::instances = -1;

NodeImpl::NodeImpl() {
	_uuid = UUID::getUUID();
	instances++;
}

NodeImpl::~NodeImpl() {
	instances--;
}

Node::Node() {
	_impl = StaticPtrCast<NodeImpl>(Factory::create("node.zmq"));
	EndPoint::_impl = _impl;
	NodeConfig options(0, 0);
	_impl->init(&options);
}

Node::Node(const NodeConfig* config) {
	_impl = StaticPtrCast<NodeImpl>(Factory::create("node.zmq"));
	EndPoint::_impl = _impl;
	_impl->init(config);

}

Node::Node(uint16_t nodePort, uint16_t pubPort) {
	_impl = StaticPtrCast<NodeImpl>(Factory::create("node.zmq"));
	EndPoint::_impl = _impl;
	NodeConfig config(nodePort, pubPort);
	_impl->init(&config);
}

Node::~Node() {
}

void Node::connect(Connectable* conn) {
	std::map<std::string, Subscriber> subs = conn->getSubscribers();
	std::map<std::string, Subscriber>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		addSubscriber(subIter->second);
		subIter++;
	}
	std::map<std::string, Publisher> pubs = conn->getPublishers();
	std::map<std::string, Publisher>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		addPublisher(pubIter->second);
		pubIter++;
	}
	conn->addedToNode(*this);
}

void Node::disconnect(Connectable* conn) {
	std::map<std::string, Publisher> pubs = conn->getPublishers();
	std::map<std::string, Publisher>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		removePublisher(pubIter->second);
		pubIter++;
	}
	std::map<std::string, Subscriber> subs = conn->getSubscribers();
	std::map<std::string, Subscriber>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		removeSubscriber(subIter->second);
		subIter++;
	}
	conn->removedFromNode(*this);
}

std::ostream & operator<<(std::ostream &os, const EndPoint& endPoint) {
	if (endPoint.isRemote()) {
		os << "remote";
	} else {
		if (endPoint.isInProcess()) {
			os << "inproc";
		} else {
			os << "local";
		}
	}
	os << "." << endPoint.getTransport() << "://" << endPoint.getIP() << ":" << endPoint.getPort() << " aka " << endPoint.getHost() << endPoint.getDomain();
	return os;
}

std::ostream& operator<<(std::ostream &out, const NodeStub* n) {
	out << SHORT_UUID(n->getUUID()) << ": ";
	out << n->getHost() << "";
	out << n->getDomain() << ":";
	out << n->getPort();

	// map<int, string>::const_iterator iFaceIter;
	// for (iFaceIter = n->_interfaces.begin(); iFaceIter != n->_interfaces.end(); iFaceIter++) {
	// 	out << "\t" << iFaceIter->first << ": " << iFaceIter->second << std::endl;
	// }
	return out;
}

}
