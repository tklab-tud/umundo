/**
 *  @file
 *  @brief      Discovery implementation with Bonjour.
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

#ifndef DISCOVERER_H_94LKA4M1
#define DISCOVERER_H_94LKA4M1

extern "C" {
#include "dns_sd.h"
}

/**
 * Keep in mind that the bonjour concept of service differs from umundo's. Bonjour services are
 * things like printers or web-servers. In this sense, we provide a umundo node as a bonjour service.
 */

#include "umundo/common/Common.h"
#include "umundo/thread/Thread.h"
#include "umundo/discovery/Discovery.h"

#define BONJOUR_REPOLL_USEC 20000
#define BONJOUR_REPOLL_SEC 0

namespace umundo {

class NodeQuery;
class BonjourNodeStub;

/**
 * Concrete discovery implementor for Bonjour (bridge pattern).
 *
 * This class is a concrete implementor (in the bridge pattern sense) for the Discovery subsystem. It uses
 * Bonjour from Apple to realize node discovery within a multicast domain.
 * \see
 *	http://developer.apple.com/library/mac/#documentation/Networking/Reference/DNSServiceDiscovery_CRef/dns_sd_h/<br />
 *  http://developer.apple.com/library/mac/#documentation/networking/Conceptual/dns_discovery_api/Introduction.html<br />
 *  http://developer.apple.com/opensource/
 */
class DLLEXPORT BonjourNodeDiscovery : public DiscoveryImpl, public Thread {
public:
	virtual ~BonjourNodeDiscovery();
	static shared_ptr<BonjourNodeDiscovery> getInstance();  ///< Return the singleton instance.

	shared_ptr<Implementation> create();
	void init(shared_ptr<Configuration>);
	void suspend();
	void resume();

	void add(NodeImpl* node);
	void remove(NodeImpl* node);

	void browse(shared_ptr<NodeQuery> discovery);
	void unbrowse(shared_ptr<NodeQuery> discovery);

	void run();

protected:
	BonjourNodeDiscovery();

	bool validateState();
	void forgetRemoteNodesFDs(shared_ptr<BonjourNodeStub>); ///< Remove a remote node with all its queries
	static const std::string errCodeToString(DNSServiceErrorType errType);

	/** @name Bonjour callbacks */
	//@{
	static void DNSSD_API browseReply(
	    DNSServiceRef sdref,
	    const DNSServiceFlags flags,
	    uint32_t ifIndex,
	    DNSServiceErrorType errorCode,
	    const char *replyName,
	    const char *replyType,
	    const char *replyDomain,
	    void *context
	);

	static void DNSSD_API registerReply(
	    DNSServiceRef sdRef,
	    DNSServiceFlags flags,
	    DNSServiceErrorType errorCode,
	    const char* name,
	    const char* regtype,
	    const char* domain,
	    void* context
	);

	static void DNSSD_API serviceResolveReply(
	    DNSServiceRef sdref,
	    const DNSServiceFlags flags,
	    uint32_t ifIndex,
	    DNSServiceErrorType errorCode,
	    const char *fullname,
	    const char *hosttarget,
	    uint16_t opaqueport,
	    uint16_t txtLen,
	    const unsigned char *txtRecord,
	    void *context
	);

	static void DNSSD_API addrInfoReply(
	    DNSServiceRef sdRef,
	    DNSServiceFlags flags,
	    uint32_t interfaceIndex,
	    DNSServiceErrorType errorCode,
	    const char *hostname,
	    const struct sockaddr *address,
	    uint32_t ttl,
	    void *context
	);
	//@}

	map<int, DNSServiceRef> _activeFDs;                       ///< Socket file descriptors to bonjour handle.

	map<intptr_t, NodeImpl*> _localNodes;         ///< Local node addresses to nodes.
	map<intptr_t, NodeImpl*> _suspendedNodes;     ///< Save local nodes when suspending.
	map<intptr_t, DNSServiceRef> _registerClients;            ///< local node address to bonjour handles for registration.

	map<intptr_t, shared_ptr<NodeQuery> > _queries;           ///< query address to query object for browseReply.
	map<intptr_t, DNSServiceRef> _queryClients;               ///< query address to bonjour handles.
	map<intptr_t, shared_ptr<NodeQuery> > _nodeToQuery;       ///< remote node addresses to their queries.
	map<shared_ptr<NodeQuery>, map<string, shared_ptr<BonjourNodeStub> > > _queryToNodes; ///< query to all its nodes.

	Mutex _mutex;
	Monitor _monitor;

	static shared_ptr<BonjourNodeDiscovery> _instance;  ///< The singleton instance.

	friend class BonjourNodeStub;
	friend class Factory;
};

}

#endif /* end of include guard: DISCOVERER_H_94LKA4M1 */
