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

#include "umundo/discovery/avahi/AvahiNodeStub.h"
#include "umundo/discovery/avahi/AvahiNodeDiscovery.h"

namespace umundo {

AvahiNodeStub::AvahiNodeStub() {
	_isRemote = false;
	_isInProcess = false;
};

AvahiNodeStub::~AvahiNodeStub() {
}

const std::string AvahiNodeStub::getIP() const {
	return (_interfacesIPv4.begin())->second;
}

std::ostream& operator<<(std::ostream &out, const AvahiNodeStub* n) {
	out << n->_uuid;
	return out;
}

}
