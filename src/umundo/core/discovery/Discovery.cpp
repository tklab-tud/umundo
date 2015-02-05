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

#include "umundo/core/discovery/Discovery.h"
#include "umundo/core/discovery/MDNSDiscovery.h"
#include "umundo/core/discovery/BroadcastDiscovery.h"

#include "umundo/core/Factory.h"
#include "umundo/core/connection/Node.h"

namespace umundo {

Discovery::Discovery(DiscoveryType type, const std::string domain) {
	DiscoveryConfig* config;
	switch (type) {
	case BROADCAST: {
		config = new DiscoveryConfigBCast();
		break;
	}
	default:
		config = new DiscoveryConfigMDNS();
		break;
	}

	if (domain.length() > 0) {
		config->setDomain(domain);
	} else {
		config->setDomain("local.");
	}

	init(config);
	delete config;

}

Discovery::Discovery(DiscoveryConfig* config) {
	init(config);
}

Discovery::~Discovery() {
}

void Discovery::init(DiscoveryConfig* config) {
	switch (config->_type) {
	case MDNS:
		_impl = StaticPtrCast<DiscoveryImpl>(Factory::create("discovery.mdns"));
		break;
	case BROADCAST:
		_impl = StaticPtrCast<DiscoveryImpl>(Factory::create("discovery.broadcast"));
		break;
	default:
		break;
	}
	if (_impl)
		_impl->init(config);

}


}