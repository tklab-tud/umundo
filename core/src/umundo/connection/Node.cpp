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

#include "umundo/connection/Node.h"

#include "umundo/common/Factory.h"
#include "umundo/common/UUID.h"
#include "umundo/discovery/Discovery.h"
#include "umundo/connection/Subscriber.h"
#include "umundo/connection/Publisher.h"

namespace umundo {

int NodeImpl::instances = -1;

shared_ptr<Configuration> NodeConfig::create() {
	return shared_ptr<Configuration>(new NodeConfig());
}

NodeImpl::NodeImpl() {
	_uuid = UUID::getUUID();
  instances++;
}

NodeImpl::~NodeImpl() {
  instances--;
}
  
Node::Node() {
	_impl = boost::static_pointer_cast<NodeImpl>(Factory::create("node"));
  NodeStubBase::_impl = _impl;
  EndPoint::_impl = _impl;
	shared_ptr<Configuration> config = Factory::config("node");
	_impl->init(config);
}

Node::Node(string domain) {
	_impl = boost::static_pointer_cast<NodeImpl>(Factory::create("node"));
  NodeStubBase::_impl = _impl;
  EndPoint::_impl = _impl;
	shared_ptr<Configuration> config = Factory::config("node");
	_impl->setDomain(domain);
	_impl->init(config);
	// add our node query
}

Node::~Node() {
}

void Node::connect(Connectable* conn) {
	set<Subscriber> subs = conn->getSubscribers();
	set<Subscriber>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		addSubscriber(*subIter);
		subIter++;
	}
	set<Publisher> pubs = conn->getPublishers();
	set<Publisher>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		addPublisher(*pubIter);
		pubIter++;
	}
	conn->addedToNode(*this);
}

void Node::disconnect(Connectable* conn) {
	set<Publisher> pubs = conn->getPublishers();
	set<Publisher>::iterator pubIter = pubs.begin();
	while(pubIter != pubs.end()) {
		removePublisher(*pubIter);
		pubIter++;
	}
	set<Subscriber> subs = conn->getSubscribers();
	set<Subscriber>::iterator subIter = subs.begin();
	while(subIter != subs.end()) {
		removeSubscriber(*subIter);
		subIter++;
	}
	conn->removedFromNode(*this);
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
