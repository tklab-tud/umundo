/**
 *  @file
 *  @brief      Host-specific information (e.g. NICs, HostID).
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

#ifndef HOST_H_BIUV9UT4
#define HOST_H_BIUV9UT4

#include <umundo/core/Common.h>
#include <vector>

namespace umundo {

/**
 * Representation of a network interface on this host.
 */
struct Interface {
public:
	std::vector<std::string> ipv4;
	std::vector<std::string> ipv6;
	std::string mac;
	std::string name;
};

/**
 * Retrieve host-specific information.
 */
class Host {
public:
	static const std::string getHostname();  ///< hostname
	static const std::vector<Interface> getInterfaces();  ///< get a list of all the hosts network interfaces
	static const std::string getHostId();    ///< 36 byte string unique to the host
};

}

#endif /* end of include guard: HOST_H_BIUV9UT4 */
