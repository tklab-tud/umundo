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

#include "umundo/discovery/Discovery.h"
#include "umundo/discovery/MDNSDiscovery.h"
#include "umundo/discovery/BroadcastDiscovery.h"

#include "umundo/common/Factory.h"
#include "umundo/connection/Node.h"

namespace umundo {

Discovery::Discovery(DiscoveryType type, Options* config) {
	switch (type) {
		case MDNS:
			_impl = boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery.mdns"));
			break;
		case BROADCAST:
			_impl = boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery.broadcast"));
			break;
		default:
			break;
	}
	if (_impl)
		_impl->init(config);
}

Discovery::Discovery(DiscoveryType type, const std::string domain) {
	switch (type) {
		case MDNS: {
			_impl = boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery.mdns"));
			MDNSDiscoveryOptions* config = new MDNSDiscoveryOptions();
			config->setDomain(domain);
			_impl->init(config);
			break;
		}
		case BROADCAST: {
			_impl = boost::static_pointer_cast<DiscoveryImpl>(Factory::create("discovery.broadcast"));
			BroadcastDiscoveryOptions* config = new BroadcastDiscoveryOptions();
			config->setDomain(domain);
			_impl->init(config);
			break;
		}
		default:
			break;
	}

}
	
Discovery::~Discovery() {
}


}