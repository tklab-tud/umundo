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
#include "umundo/discovery/MDNSDiscovery.h"

#define BONJOUR_REPOLL_USEC 20000
#define BONJOUR_REPOLL_SEC 0

namespace umundo {

class NodeQuery;

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
class DLLEXPORT BonjourDiscovery : public MDNSDiscoveryImpl, public Thread {
public:
	BonjourDiscovery();
	virtual ~BonjourDiscovery();
	static boost::shared_ptr<BonjourDiscovery> getInstance();  ///< Return the singleton instance.

	boost::shared_ptr<Implementation> create();
	void init(Options*);
	void suspend();
	void resume();

	void advertise(MDNSAd* node);
	void unadvertise(MDNSAd* node);
	
	void browse(MDNSQuery* query);
	void unbrowse(MDNSQuery* query);

	void run();

protected:

	bool validateState();
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

	class BonjourQuery {
	public:
		DNSServiceRef mdnsClient;
		std::set<MDNSQuery*> queries;
		std::map<std::string, MDNSAd*> remoteAds;
	};

	struct BonjourBrowseReply {
		uint32_t ifIndex;
		DNSServiceFlags flags;
    DNSServiceErrorType errorCode;
		std::string serviceName;
    std::string regtype;
    std::string domain;
    void *context;
	};
	
	struct BonjourAddrInfoReply {
		DNSServiceFlags flags;
    uint32_t interfaceIndex;
    DNSServiceErrorType errorCode;
		std::string hostname;
		std::string ipv4;
		std::string ipv6;
    uint32_t ttl;
    void *context;
	};
	
	/// All the mdns service refs for a mdns advertisement
	class BonjourServiceRefs {
	public:
		BonjourServiceRefs() : serviceRegister(NULL) {}
		DNSServiceRef serviceRegister; ///< used to register a local node
		std::map<uint32_t, DNSServiceRef> serviceResolver; ///< used to resolve a service found via browse per interface
		std::map<uint32_t, DNSServiceRef> serviceGetAddrInfo; ///< used to get the address of a resolved service
	};
	
	void dumpQueries();
	
	DNSServiceRef _mainDNSHandle;
	std::map<int, DNSServiceRef> _activeFDs;                       ///< Socket file descriptors to bonjour handle.

	/// domain to type to client with set of queries
	std::map<std::string, std::map<std::string, BonjourQuery> > _queryClients;
	std::map<MDNSAd*, BonjourServiceRefs> _localAds;
	std::map<MDNSAd*, BonjourServiceRefs> _remoteAds;
	
	std::list<BonjourAddrInfoReply> _pendingAddrInfoReplies;
	std::list<BonjourBrowseReply> _pendingBrowseReplies;
	
	int _nodes;
	int _ads;
	
	Mutex _mutex;
	Monitor _monitor;

	static boost::shared_ptr<BonjourDiscovery> _instance;  ///< The singleton instance.

	friend class BonjourNodeStub;
	friend class Factory;
};

}

#endif /* end of include guard: DISCOVERER_H_94LKA4M1 */
