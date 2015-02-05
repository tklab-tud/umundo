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

#ifndef BONJOURDISCOVERER_H_D5FC35F7
#define BONJOURDISCOVERER_H_D5FC35F7

extern "C" {
#include "dns_sd.h"
}

/**
 * Keep in mind that the bonjour concept of service differs from umundo's. Bonjour services are
 * things like printers or web-servers. In this sense, we provide a umundo node as a bonjour service.
 */

#include "umundo/core/Common.h"
#include "umundo/core/thread/Thread.h"
#include "umundo/core/discovery/MDNSDiscovery.h"

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
class UMUNDO_API BonjourDiscoverer : public MDNSDiscoveryImpl, public Thread {
public:
	BonjourDiscoverer();
	virtual ~BonjourDiscoverer();
	static SharedPtr<BonjourDiscoverer> getInstance();  ///< Return the singleton instance.

	SharedPtr<Implementation> create();
	void init(const Options*);
	void suspend();
	void resume();

	void advertise(MDNSAdvertisement* node);
	void unadvertise(MDNSAdvertisement* node);

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

	struct NativeBonjourBrowseReply {
		uint32_t ifIndex;
		DNSServiceFlags flags;
		DNSServiceErrorType errorCode;
		std::string serviceName;
		std::string regtype;
		std::string domain;
		void *context;
	};

	struct NativeBonjourAddrInfoReply {
		DNSServiceFlags flags;
		uint32_t interfaceIndex;
		DNSServiceErrorType errorCode;
		std::string hostname;
		std::string ipv4;
		std::string ipv6;
		uint32_t ttl;
		void *context;
	};

	/** @name Representation of a native query on bonjour */
	//@{

	class NativeBonjourQuery {
	public:
		NativeBonjourQuery() : serviceBrowseDNSClient(NULL) {}
		DNSServiceRef serviceBrowseDNSClient; ///< browser for a type in a domain
		std::set<MDNSQuery*> queries;
		std::map<std::string, MDNSAdvertisement*> remoteAds; // relevant subset of remote ads from _remoteAds below
	};

	/// domain to type to client with set of queries
	std::map<std::string, std::map<std::string, NativeBonjourQuery> > _queryClients;

	bool hasNativeQueryInDomainForType(const std::string& domain, const std::string& type) {
		if (_queryClients.find(domain) == _queryClients.end() ||
		        _queryClients[domain].find(type) == _queryClients[domain].end() ||
		        _queryClients[domain][type].queries.size() == 0) {
			return false;
		}
		return true;
	}

	//@}

	/// All the mdns service refs for a mdns advertisement
	class NativeBonjourServiceRefs {
	public:
		NativeBonjourServiceRefs() : serviceRegisterDNSClient(NULL) {}
		DNSServiceRef serviceRegisterDNSClient; ///< used to register a local node for advertisement
		std::map<uint32_t, DNSServiceRef> serviceResolverOnIface; ///< used to resolve a service found via browse per interface
		std::map<uint32_t, DNSServiceRef> serviceGetAddrInfoOnIface; ///< used to get the address of a resolved service
	};

	void dumpQueries();

	DNSServiceRef _mainDNSHandle;
	std::map<int, DNSServiceRef> _activeFDs;                       ///< Socket file descriptors to bonjour handle.

	std::map<MDNSAdvertisement*, NativeBonjourServiceRefs> _localAds;
	std::map<MDNSAdvertisement*, NativeBonjourServiceRefs> _remoteAds;

	std::list<NativeBonjourAddrInfoReply> _pendingAddrInfoReplies;
	std::list<NativeBonjourBrowseReply> _pendingBrowseReplies;

	int _nodes;
	int _ads;

	RMutex _mutex;
	Monitor _monitor;

	static WeakPtr<BonjourDiscoverer> _instance;  ///< The singleton instance.

	friend class BonjourNodeStub;
	friend class Factory;
};

}

#endif /* end of include guard: BONJOURDISCOVERER_H_D5FC35F7 */
