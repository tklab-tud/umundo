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

#ifndef HOST_H_BIUV9UT4
#define HOST_H_BIUV9UT4

#include <umundo/common/Common.h>
#include <vector>

namespace umundo {
  
struct Interface {
public:
  set<string> ipv4;
  set<string> ipv6;
  string mac;
  string name;
};
  
class Host {
public:
	static const string& getHostname();  ///< hostname
	static const vector<Interface> getInterfaces();  ///< get a list of all the hosts network interfaces
	static const string& getHostId();    ///< 36 byte string unique to the host
};
	
}

#endif /* end of include guard: HOST_H_BIUV9UT4 */
