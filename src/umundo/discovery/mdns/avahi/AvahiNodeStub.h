/**
 *  @file
 *  @brief      Remote Avahi Node representation.
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

#ifndef AVAHINODESTUB_H_ACCARM71
#define AVAHINODESTUB_H_ACCARM71

#include "umundo/connection/Node.h"

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>

#include <sys/socket.h>
#include <netinet/in.h>

namespace umundo {

class AvahiNodeStubDiscovery;

/**
 * Concrete nodestub implementor for avahi (bridge pattern).
 */
class UMUNDO_API AvahiNodeStub : public NodeStubImpl {
public:
	AvahiNodeStub();
	virtual ~AvahiNodeStub();

	/// Overwritten from EndPoint.
	const std::string getIP() const;

private:
	void resolve();

	bool _isOurOwn;
	bool _isWan;
	string _txtRecord;

	map<int, string> _interfacesIPv4;
	map<int, string> _interfacesIPv6;

	std::set<string> _domains;
	std::set<int> _interfaceIndices;

	friend std::ostream& operator<<(std::ostream&, const AvahiNodeStub*);
	friend class AvahiNodeDiscovery;
};

}

#endif /* end of include guard: AVAHINODESTUB_H_ACCARM71 */
