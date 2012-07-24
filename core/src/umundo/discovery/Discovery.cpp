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

#include "umundo/discovery/Discovery.h"

#include "umundo/common/Factory.h"
#include "umundo/connection/Node.h"

namespace umundo {

/**
 * @todo: This never gets called if we use the static functions - refactor
 */
Discovery::Discovery() {
	_impl = boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery"));
	shared_ptr<DiscoveryConfig> config = boost::static_pointer_cast<DiscoveryConfig>(Factory::config("discovery"));
	_impl->init(config);
}

Discovery::~Discovery() {
}

void Discovery::remove(Node* node) {
	boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery"))->remove(node->_impl);
}

void Discovery::add(Node* node) {
	boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery"))->add(node->_impl);
}

void Discovery::browse(shared_ptr<NodeQuery> discovery) {
	boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery"))->browse(discovery);
}

void Discovery::unbrowse(shared_ptr<NodeQuery> discovery) {
	boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery"))->unbrowse(discovery);
}

}