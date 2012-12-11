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
#include "umundo/discovery/NodeQuery.h"
#include "umundo/common/Factory.h"

namespace umundo {

NodeQuery::NodeQuery(string domain, ResultSet<NodeStub>* listener) :
	_notifyImmediately(true), _domain(domain), _transport("tcp"), _listener(listener) {
}

NodeQuery::~NodeQuery() {
}

void NodeQuery::found(const NodeStub& node) {
	ScopeLock lock(_mutex);
	assert(node);
	if (_notifyImmediately) {
		if (_nodes.find(node.getUUID()) != _nodes.end()) {
//      LOG_DEBUG("Changed node %s", SHORT_UUID(node->getUUID()).c_str());
			_listener->changed(node);
		} else {
			LOG_DEBUG("Added node %s", SHORT_UUID(node.getUUID()).c_str());
			_listener->added(node);
			_nodes[node.getUUID()] = node;
		}
	} else {
		_pendingFinds.insert(node);
	}
}

void NodeQuery::removed(const NodeStub& node) {
	ScopeLock lock(_mutex);
	assert(node);
	if (_notifyImmediately) {
		LOG_DEBUG("Removed node %s", SHORT_UUID(node.getUUID()).c_str());
		_listener->removed(node);
		_nodes.erase(node.getUUID());
	} else {
		_pendingRemovals.insert(node);
	}
}

void NodeQuery::notifyImmediately(bool notifyImmediately) {
	_notifyImmediately = notifyImmediately;
}

void NodeQuery::notifyResultSet() {
	ScopeLock lock(_mutex);
	set<NodeStub>::const_iterator nodeIter;

	for (nodeIter = _pendingRemovals.begin(); nodeIter != _pendingRemovals.end(); nodeIter++) {
		assert(*nodeIter);
		LOG_DEBUG("Removed node %s", SHORT_UUID(nodeIter->getUUID()).c_str());
		_listener->removed(*nodeIter);
		_nodes.erase(nodeIter->getUUID());
	}

	for (nodeIter = _pendingFinds.begin(); nodeIter != _pendingFinds.end(); nodeIter++) {
		assert(*nodeIter);
		if (_nodes.find(nodeIter->getUUID()) != _nodes.end()) {
//      LOG_DEBUG("Changed node %s", SHORT_UUID((*nodeIter)->getUUID()).c_str());
			_listener->changed(NodeStub(*nodeIter));
		} else {
			LOG_DEBUG("Added node %s", SHORT_UUID(nodeIter->getUUID()).c_str());
			_listener->added(*nodeIter);
			_nodes[nodeIter->getUUID()] = *nodeIter;
		}
	}

	_pendingRemovals.clear();
	_pendingFinds.clear();
}

const string& NodeQuery::getDomain() {
	return _domain;
}

void NodeQuery::setTransport(string transport) {
	_transport = transport;
}

const string& NodeQuery::getTransport() {
	return _transport;
}

}