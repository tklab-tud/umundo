/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
 */

#include "umundo/connection/Node.h"

#include "umundo/common/Factory.h"
#include "umundo/common/UUID.h"
#include "umundo/discovery/Discovery.h"
#include "umundo/connection/Subscriber.h"
#include "umundo/connection/Publisher.h"

namespace umundo {

shared_ptr<Configuration> NodeConfig::create() {
	return shared_ptr<Configuration>(new NodeConfig());
}

NodeImpl::NodeImpl() {
	_uuid = UUID::getUUID();
}

int Node::instances = 0;
Node::Node() {
	_impl = boost::static_pointer_cast<NodeImpl>(Factory::create("node"));
	shared_ptr<Configuration> config = Factory::config("node");
	_impl->init(config);
	// add our node query
	Discovery::add(this);
	instances++;
}

Node::Node(string domain) {
	_impl = boost::static_pointer_cast<NodeImpl>(Factory::create("node"));
	shared_ptr<Configuration> config = Factory::config("node");
	_impl->setDomain(domain);
	_impl->init(config);
	Discovery::add(this);
	instances++;
}

Node::~Node() {
	Discovery::remove(this);
	instances--;
}

void Node::addSubscriber(Subscriber* sub) {
	_impl->addSubscriber(sub->_impl);
}

void Node::removeSubscriber(Subscriber* sub) {
	_impl->removeSubscriber(sub->_impl);
}

void Node::addPublisher(Publisher* pub) {
	_impl->addPublisher(pub->_impl);
}

void Node::removePublisher(Publisher* pub) {
	_impl->removePublisher(pub->_impl);
}

void Node::connect(Connectable* conn) {
	set<Subscriber*> subs = conn->getSubscribers();
	set<Subscriber*>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		_impl->addSubscriber((*subIter)->_impl);
		subIter++;
	}
	set<Publisher*> pubs = conn->getPublishers();
	set<Publisher*>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		_impl->addPublisher((*pubIter)->_impl);
		pubIter++;
	}
	conn->addedToNode(this);
}

void Node::disconnect(Connectable* conn) {
	set<Publisher*> pubs = conn->getPublishers();
	set<Publisher*>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		_impl->removePublisher((*pubIter)->_impl);
		pubIter++;
	}
	set<Subscriber*> subs = conn->getSubscribers();
	set<Subscriber*>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		_impl->removeSubscriber((*subIter)->_impl);
		subIter++;
	}
	conn->removedFromNode(this);
}

void Node::suspend() {
	_impl->suspend();
}

void Node::resume() {
	_impl->resume();
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
