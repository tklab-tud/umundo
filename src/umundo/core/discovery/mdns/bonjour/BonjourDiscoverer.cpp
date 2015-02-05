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

#define NO_DEBUG_MSGS

#include "umundo/config.h"

#ifdef WIN32
#include <WS2tcpip.h>
#endif

/** from dns_sd.h:kDNSServiceFlagsShareConnection
 * When callbacks are invoked using a shared DNSServiceRef, the
 * kDNSServiceFlagsMoreComing flag applies collectively to *all* active
 * operations sharing the same parent DNSServiceRef. If the MoreComing flag is
 * set it means that there are more results queued on this parent DNSServiceRef,
 * but not necessarily more results for this particular callback function.
 */

#if 1
#	undef SHARE_SERVICE_REF
# define SHARE_SERVICE_FLAG 0
#else
# define SHARE_SERVICE_REF 1
# define SHARE_SERVICE_FLAG kDNSServiceFlagsShareConnection
#endif

// we do not have FDs with embedded bonjour at all,
// with non-embedded, we might share them
#if not defined(DISC_BONJOUR_EMBED) and not defined(SHARE_SERVICE_REF)
#define NON_SHARED_FDS 1
#endif

#include "umundo/core/discovery/mdns/bonjour/BonjourDiscoverer.h"

#include <errno.h>

#if (defined UNIX || defined IOS || defined ANDROID)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // gethostname
#include <stdio.h>
#include <string.h>
#endif

#include "umundo/core/connection/Node.h"

#ifdef DISC_BONJOUR_EMBED
extern "C" {
	/// See mDNSEmbedded.c
	int embedded_mDNSInit();
	void embedded_mDNSExit();
	int embedded_mDNSmainLoop(timeval);
}
#endif

namespace umundo {

SharedPtr<Implementation> BonjourDiscoverer::create() {
	return getInstance();
}

void BonjourDiscoverer::init(const Options*) {
	// do nothing?
}

SharedPtr<BonjourDiscoverer> BonjourDiscoverer::getInstance() {
	SharedPtr<BonjourDiscoverer> instance = _instance.lock();

	if (!instance) {
		instance = SharedPtr<BonjourDiscoverer>(new BonjourDiscoverer());
#ifdef DISC_BONJOUR_EMBED
		UM_LOG_DEBUG("Initializing embedded mDNS server");
		int err = embedded_mDNSInit();
		if (err) {
			UM_LOG_WARN("mDNS_Init returned error: %s", errCodeToString(err).c_str());
		} else {
			instance->start();
		}
#else

#ifdef SHARE_SERVICE_REF
		DNSServiceErrorType error = DNSServiceCreateConnection(&instance->_mainDNSHandle);
		if (error) {
			UM_LOG_WARN("DNSServiceCreateConnection returned error: %s", errCodeToString(error).c_str());
		} else {
			instance->_activeFDs[DNSServiceRefSockFD(instance->_mainDNSHandle)] = instance->_mainDNSHandle;
			instance->start();
		}
#endif
		instance->start();

#endif
		_instance = instance;
	}
	return instance;
}
WeakPtr<BonjourDiscoverer> BonjourDiscoverer::_instance;

BonjourDiscoverer::BonjourDiscoverer() {
	_mainDNSHandle = NULL;
	_nodes = 0;
	_ads = 0;
}

BonjourDiscoverer::~BonjourDiscoverer() {
#if 0
	{
		RScopeLock lock(_mutex);
		map<intptr_t, Node >::iterator nodeIter = _localNodes.begin();
		while(nodeIter != _localNodes.end()) {
			Node node = nodeIter->second;
			nodeIter++;
			remove(node);
		}
	}
#endif
	stop();
	join(); // we have deadlock in embedded?

	//	while (isJoinable()) { // CSharp bindings croaks on Thread::sleep in run?
	//		Thread::sleepMs(2000);
	//		std::cout << "Still trying to join" << std::endl;
	//		join();
	//	}

#ifdef DISC_BONJOUR_EMBED
	// notify every other host that we are about to vanish
	embedded_mDNSExit();
#endif
	if (_mainDNSHandle) {
		DNSServiceRefDeallocate(_mainDNSHandle);
	}
}

/**
 * Unregister all nodes when suspending.
 *
 * When we are suspended, we unadvertise all registered nodes and save them as
 * _suspendedNodes to add them when resuming.
 */
void BonjourDiscoverer::suspend() {
	RScopeLock lock(_mutex);
	if (_isSuspended) {
		return;
	}
	_isSuspended = true;
#if 0
	// remove all nodes from discovery
	_suspendedNodes = _localNodes;
	map<intptr_t, Node>::iterator nodeIter = _localNodes.begin();
	while(nodeIter != _localNodes.end()) {
		Node node = nodeIter->second;
		nodeIter++;
		remove(node);
	}
#endif
}

/**
 * Re-register nodes previously suspended.
 *
 * We remembered all our nodes in _suspendedNodes when this object was suspended.
 * By explicitly calling resume on the nodes we add again, we make sure they are
 * reinitialized before advertising them.
 */
void BonjourDiscoverer::resume() {
	RScopeLock lock(_mutex);
	if (!_isSuspended) {
		return;
	}
	_isSuspended = false;
#if 0
	map<intptr_t, Node>::iterator nodeIter = _suspendedNodes.begin();
	while(nodeIter != _suspendedNodes.end()) {
		Node node = nodeIter->second;
		nodeIter++;
		// make sure advertised nodes are initialized
		node.resume();
		add(node);
	}
	_suspendedNodes.clear();
#endif
}

void BonjourDiscoverer::run() {
	struct timeval tv;

#ifdef DISC_BONJOUR_EMBED
	while(isStarted()) {
		UMUNDO_LOCK(_mutex);
		tv.tv_sec  = BONJOUR_REPOLL_SEC;
		tv.tv_usec = BONJOUR_REPOLL_USEC;
		embedded_mDNSmainLoop(tv);
		UMUNDO_UNLOCK(_mutex);
		Thread::sleepMs(50);
	}

#else

	fd_set readfds;
	int nfds = -1;

	UM_LOG_DEBUG("Beginning select");
	while(isStarted()) {
		nfds = 0;
		FD_ZERO(&readfds);
		// tv structure gets modified we have to reset each time
		tv.tv_sec  = BONJOUR_REPOLL_SEC;
		tv.tv_usec = BONJOUR_REPOLL_USEC;

		int result;
		{
			RScopeLock lock(_mutex);
			// initialize file desriptor set for select
			std::map<int, DNSServiceRef>::const_iterator cIt;
			for (cIt = _activeFDs.begin(); cIt != _activeFDs.end(); cIt++) {
				if (cIt->first == -1)
					continue;
				if (cIt->first > nfds)
					nfds = cIt->first;
				FD_SET(cIt->first, &readfds);
			}
			nfds++;
			result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);
		}
		// dispatch over result, make sure to unlock mutex everywhere
		if (result > 0) {
			std::map<int, DNSServiceRef>::iterator it;

			RScopeLock lock(_mutex);
			it = _activeFDs.begin();
			while(it != _activeFDs.end()) {
				DNSServiceRef sdref = it->second;
				int sockFD = it->first;
				it++;
				if (sockFD == -1)
					continue;
				if (FD_ISSET(sockFD, &readfds)) {
					FD_CLR(sockFD, &readfds);
					DNSServiceProcessResult(sdref);
					it = _activeFDs.begin();
				}
			}
			//      _monitor.signal();

		} else if (result == 0) {
			// timeout as no socket is selectable, just retry
		} else {
			if (errno != 0)
				UM_LOG_WARN("select failed %s", strerror(errno));
			if (nfds > FD_SETSIZE)
				UM_LOG_WARN("number of file descriptors too large: %d of %d", nfds, FD_SETSIZE);
		}

	}
#endif
}

/**
 * Add a node to be discoverable by other nodes.
 */
void BonjourDiscoverer::advertise(MDNSAdvertisement* node) {
	RScopeLock lock(_mutex);

	DNSServiceErrorType err;
	DNSServiceRef registerClient = _mainDNSHandle;

	if (_localAds.find(node) != _localAds.end()) {
		UM_LOG_WARN("Ignoring addition of node already added to discovery");
		return;
	}

	const char* domain = (node->domain.length() == 0 ? NULL : node->domain.c_str());
	const char* host = (node->host.length() == 0 ? NULL : node->host.c_str());
	const char* name = (node->name.length() == 0 ? NULL : node->name.c_str());
	const char* regType = node->regType.c_str();
	uint16_t port = htons(node->port);

	// prepare txtRecord
	uint16_t txtLen = 0;
	std::stringstream txtSS;
	for (std::set<std::string>::iterator txtIter = node->txtRecord.begin();
	        txtIter != node->txtRecord.end();
	        txtIter++) {
		if (txtIter->length() > 255) {
			if (txtLen + 255 > 0xffff)
				break;
			txtLen += 255;
			txtSS << (char)255;
			txtSS << txtIter->substr(0,255);
		} else {
			if (txtLen + txtIter->length() > 0xffff)
				break;
			txtLen += txtIter->length();
			txtSS << (char)txtIter->length();
			txtSS << *txtIter;
		}
	}

	const char* txtRData = (txtSS.str().size() > 0 ? txtSS.str().data() : NULL);

	err = DNSServiceRegister(&registerClient,                 // uninitialized DNSServiceRef
	                         SHARE_SERVICE_FLAG,              // renaming behavior on name conflict (kDNSServiceFlagsNoAutoRename)
	                         kDNSServiceInterfaceIndexAny,    // If non-zero, specifies the interface, defaults to all interfaces
	                         name,                            // If non-NULL, specifies the service name, defaults to computer name
	                         regType,                         // service type followed by the protocol
	                         domain,                          // If non-NULL, specifies the domain, defaults to default domain
	                         host,                            // If non-NULL, specifies the SRV target host name
	                         port,                            // port number, defaults to name-reserving/non-discoverable
	                         txtLen,                          // length of the txtRecord, in bytes
	                         txtRData,                        // TXT record rdata: <length byte> <data> <length byte> <data> ...
	                         registerReply,                   // called when the registration completes
	                         (void*)NULL);                    // context pointer which is passed to the callback

	if(registerClient && err == 0) {
		_ads++;
		_localAds[node].serviceRegisterDNSClient = registerClient;

#ifdef NON_SHARED_FDS
		_activeFDs[DNSServiceRefSockFD(registerClient)] = registerClient;
#endif
	}	else {
		UM_LOG_ERR("Cannot advertise node: %s", errCodeToString(err).c_str());
	}
}

void BonjourDiscoverer::unadvertise(MDNSAdvertisement* node) {
	RScopeLock lock(_mutex);

	if (_localAds.find(node) == _localAds.end()) {
		UM_LOG_WARN("Ignoring removal of unregistered node from discovery");
		return;
	}

	_activeFDs.erase(DNSServiceRefSockFD(_localAds[node].serviceRegisterDNSClient)); // noop in embedded

	// it will not have been added to _localAds if we did not get a MDNS handle
	assert(_localAds[node].serviceRegisterDNSClient != NULL);
#ifdef NON_SHARED_FDS
	_activeFDs.erase(DNSServiceRefSockFD(_localAds[node].serviceRegisterDNSClient));
#endif
	DNSServiceRefDeallocate(_localAds[node].serviceRegisterDNSClient);
	_localAds.erase(node);
	// this is required for browsereply to report vanished nodes?
	//	DNSServiceProcessResult(_mainDNSHandle);
	_ads--;
}


/**
 * Start to browse for other nodes.
 *
 * If we find a node we will try to resolve its hostname and if we found its hostname,
 * we will resolve its ip address.
 */
void BonjourDiscoverer::browse(MDNSQuery* query) {
	RScopeLock lock(_mutex);

	//	dumpQueries();
	NativeBonjourQuery& nativeQuery = _queryClients[query->domain][query->regType];

	// do we already browse for this regtype in this domain?
	if (nativeQuery.queries.size() > 0) {
		// yes, we already know all services of this type in this domain

		// do we already browse for this very query instance?
		if (nativeQuery.queries.find(query) != nativeQuery.queries.end()) {
			UM_LOG_WARN("Already browsing for given query");
			return;
		}

		nativeQuery.queries.insert(query);
		// report existing endpoints immediately
		for (std::map<std::string, MDNSAdvertisement*>::iterator adIter = nativeQuery.remoteAds.begin(); adIter != nativeQuery.remoteAds.end(); adIter++) {
			if (adIter->second->ipv4.size() > 0 || adIter->second->ipv6.size() > 0) {
				// other advertisements are not yet added in addrInfoReply
				query->rs->add(adIter->second, toStr(this));
			}
		}

	} else {
		// no not browsing for given type in given domain -> start a new query

		DNSServiceErrorType err;
		DNSServiceRef queryClient = _mainDNSHandle;

		const char* regType = (query->regType.size() > 0 ? query->regType.c_str() : NULL);
		const char* domain = (query->domain.size() > 0 ? query->domain.c_str() : NULL);

		err = DNSServiceBrowse(&queryClient,                    // uninitialized DNSServiceRef
		                       SHARE_SERVICE_FLAG,              // Currently ignored, reserved for future use
		                       kDNSServiceInterfaceIndexAny,    // non-zero, specifies the interface
		                       regType,                         // service type being browsed
		                       domain,                          // non-NULL, specifies the domain
		                       browseReply,                     // called when a service is found
		                       NULL);

		// add in any case and remove via unbrowse if we failed
		nativeQuery.queries.insert(query);
		nativeQuery.serviceBrowseDNSClient = queryClient;

		if(queryClient && err == 0) {
#ifdef NON_SHARED_FDS
			_activeFDs[DNSServiceRefSockFD(queryClient)] = queryClient;
#endif
		} else {
			UM_LOG_ERR("Cannot browse for type '%s' in domain '%s' given query: %s", query->regType.c_str(), query->domain.c_str(), errCodeToString(err).c_str());
			// something went wrong, unbrowse query
			unbrowse(query);
		}

	}
	return;
}

void BonjourDiscoverer::unbrowse(MDNSQuery* query) {
	RScopeLock lock(_mutex);

	if (!hasNativeQueryInDomainForType(query->domain, query->regType)) {
		UM_LOG_WARN("Cannot unbrowse: No one is browsing for %s in %s", query->regType.c_str(), query->domain.c_str());
	}

	NativeBonjourQuery& nativeQuery = _queryClients[query->domain][query->regType];

	std::set<MDNSQuery*>::iterator queryIter = nativeQuery.queries.find(query);
	if (queryIter == nativeQuery.queries.end()) {
		UM_LOG_WARN("Unbrowsing query that was never added");
		return;
	}

	// remove query
	nativeQuery.queries.erase(queryIter);

	if (nativeQuery.queries.size() == 0) {
		// last browser is gone, remove this native query
		DNSServiceRef queryClient = nativeQuery.serviceBrowseDNSClient;
		if (queryClient) {
#ifdef NON_SHARED_FDS
			_activeFDs.erase(DNSServiceRefSockFD(queryClient));
#endif
			DNSServiceRefDeallocate(queryClient);
		}

		// tidy up domain -> type map
		_queryClients[query->domain].erase(query->regType);
		if (_queryClients[query->domain].size() == 0) {
			_queryClients.erase(query->domain);
		}
	}
}

/**
 * There has been an answer to one of our queries.
 *
 * Gather all additions, changes and removals and start service resolving.
 */
void DNSSD_API BonjourDiscoverer::browseReply(DNSServiceRef sdref_,
        const DNSServiceFlags flags_,
        uint32_t ifIndex_,
        DNSServiceErrorType errorCode_,
        const char *serviceName_,
        const char *regtype_,
        const char *domain_,
        void *context_) {

	/* Browse for instances of a service.
	 *
	 * DNSServiceBrowseReply() Parameters:
	 *
	 * sdRef:           The DNSServiceRef initialized by DNSServiceBrowse().
	 *
	 * flags:           Possible values are kDNSServiceFlagsMoreComing and kDNSServiceFlagsAdd.
	 *                  See flag definitions for details.
	 *
	 * interfaceIndex:  The interface on which the service is advertised. This index should
	 *                  be passed to DNSServiceResolve() when resolving the service.
	 *
	 * errorCode:       Will be kDNSServiceErr_NoError (0) on success, otherwise will
	 *                  indicate the failure that occurred. Other parameters are undefined if
	 *                  the errorCode is nonzero.
	 *
	 * serviceName:     The discovered service name. This name should be displayed to the user,
	 *                  and stored for subsequent use in the DNSServiceResolve() call.
	 *
	 * regtype:         The service type, which is usually (but not always) the same as was passed
	 *                  to DNSServiceBrowse(). One case where the discovered service type may
	 *                  not be the same as the requested service type is when using subtypes:
	 *                  The client may want to browse for only those ftp servers that allow
	 *                  anonymous connections. The client will pass the string "_ftp._tcp,_anon"
	 *                  to DNSServiceBrowse(), but the type of the service that's discovered
	 *                  is simply "_ftp._tcp". The regtype for each discovered service instance
	 *                  should be stored along with the name, so that it can be passed to
	 *                  DNSServiceResolve() when the service is later resolved.
	 *
	 * domain:          The domain of the discovered service instance. This may or may not be the
	 *                  same as the domain that was passed to DNSServiceBrowse(). The domain for each
	 *                  discovered service instance should be stored along with the name, so that
	 *                  it can be passed to DNSServiceResolve() when the service is later resolved.
	 *
	 * context:         The context pointer that was passed to the callout.
	 *
	 */

	SharedPtr<BonjourDiscoverer> myself = getInstance();
	RScopeLock lock(myself->_mutex);

	UM_LOG_DEBUG("browseReply: received info on %s/%s as %s at if %d as %s (%d)",
	             serviceName_, domain_, regtype_, ifIndex_, (flags_ & kDNSServiceFlagsAdd ? "added" : "removed"), flags_);

	if (errorCode_ != 0) {
		UM_LOG_ERR("browseReply called with error: %s", errCodeToString(errorCode_).c_str());
		return;
	}

	NativeBonjourBrowseReply reply;
	reply.flags = flags_;
	reply.ifIndex = ifIndex_;
	reply.errorCode = errorCode_;
	reply.serviceName = serviceName_;
	reply.regtype = regtype_;
	reply.domain = domain_;
	reply.context = context_;

	// just enqueue and wait for last reply
	myself->_pendingBrowseReplies.push_back(reply);

	/**
	 * There are no more pending invocations for now, process what we got
	 *
	 * Changed and removed nodes get reported directly, new nodes are resolved
	 * via serviceResolveReply and addrInfoReply first to get a qualified node
	 */
	if (~flags_ & kDNSServiceFlagsMoreComing) {
		std::map<std::string, MDNSAdvertisement*> removed; // complete service vanished
		std::map<std::string, MDNSAdvertisement*> changed; // some interface added or vanished

		std::list<NativeBonjourBrowseReply>::iterator replyIter = myself->_pendingBrowseReplies.begin();
		while(replyIter != myself->_pendingBrowseReplies.end()) {

			NativeBonjourBrowseReply& browseReply = *replyIter;

			// first some general validation
			if (myself->_queryClients.find(browseReply.domain) == myself->_queryClients.end()) {
				UM_LOG_ERR("Ignoring browseReply for domain no longer watched: %s", browseReply.domain.c_str());
				replyIter++;
				continue;
			}

			std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[browseReply.domain].find(browseReply.regtype);
			if (queryIter == myself->_queryClients[browseReply.domain].end()) {
				UM_LOG_ERR("Ignoring browseReply for service type no longer watched: %s", browseReply.regtype.c_str());
				replyIter++;
				continue;
			}
			// ok, now this is actually relevant to us

			NativeBonjourQuery& query = myself->_queryClients[browseReply.domain][browseReply.regtype];

			if (browseReply.flags & kDNSServiceFlagsAdd) {
				MDNSAdvertisement* ad;

				// is this a brand new service?
				if (query.remoteAds.find(browseReply.serviceName) == query.remoteAds.end()) {
					// brand spanking new!
					ad = new MDNSAdvertisement();
					ad->domain = browseReply.domain;
					ad->name = browseReply.serviceName;
					ad->regType = browseReply.regtype;
					query.remoteAds[browseReply.serviceName] = ad;
				} else {
					// service already known, but maybe on a new interface?
					ad = query.remoteAds[browseReply.serviceName];
				}

				assert(query.remoteAds.find(browseReply.serviceName) != query.remoteAds.end());
				assert(ad);

				// With somewhat unorthodox NIC configurations, services are reported multiple times on the same interface
				if (ad->interfaces.find(browseReply.ifIndex) != ad->interfaces.end()) {
					UM_LOG_WARN("Service '%s' at interface %d was already reported!", browseReply.serviceName.c_str(), browseReply.ifIndex);
					replyIter++;
					continue;
				}

				// a service was added, just resolve the service and do not report anything
				DNSServiceRef serviceResolveRef = myself->_mainDNSHandle;

				// remember that we have a new interface
				assert(ad->interfaces.find(browseReply.ifIndex) == ad->interfaces.end());
				ad->interfaces.insert(browseReply.ifIndex);
				ad->lastChange |= Discovery::IFACE_ADDED;

				UM_LOG_DEBUG("browseReply called for new node %p - resolving service", ad);

				// Resolve service domain name, target hostname, port number and txt record
				int err = DNSServiceResolve(&serviceResolveRef,
				                            SHARE_SERVICE_FLAG,
				                            browseReply.ifIndex,
				                            browseReply.serviceName.c_str(),
				                            browseReply.regtype.c_str(),
				                            browseReply.domain.c_str(),
				                            serviceResolveReply,
				                            (void*)ad);

				if (err != kDNSServiceErr_NoError) {
					// something went wrong
					UM_LOG_ERR("DNSServiceResolve returned with error: %s", errCodeToString(err).c_str());
					replyIter++;
					continue;
				} else {
					myself->_nodes++;
					// remember service
					assert(myself->_remoteAds[ad].serviceResolverOnIface.find(browseReply.ifIndex) == myself->_remoteAds[ad].serviceResolverOnIface.end());
					myself->_remoteAds[ad].serviceResolverOnIface[browseReply.ifIndex] = serviceResolveRef;
#ifdef NON_SHARED_FDS
					myself->_activeFDs[DNSServiceRefSockFD(serviceResolveRef)] = serviceResolveRef;
#endif

				}

			} else {
				// a service at an interface was removed
				if(query.remoteAds.find(browseReply.serviceName) == query.remoteAds.end()) {
					UM_LOG_DEBUG("browseReply: ignoring removal of unknown service");
					replyIter++;
					continue;
				}

				MDNSAdvertisement* ad = query.remoteAds[browseReply.serviceName];

				if(ad->interfaces.find(browseReply.ifIndex) == ad->interfaces.end()) {
					UM_LOG_DEBUG("browseReply: ignoring removal of unknown interface");
					replyIter++;
					continue;
				}

				ad->lastChange |= Discovery::IFACE_REMOVED;

				myself->_nodes--;

				changed[ad->name] = ad;

				// which ip address vanished?
				ad->ipv4.erase(browseReply.ifIndex);
				ad->ipv6.erase(browseReply.ifIndex);
				ad->interfaces.erase(browseReply.ifIndex);

				// last interface gone, this service vanished!
				if (ad->interfaces.size() == 0) {
					assert(changed.find(ad->name) != changed.end());
					// move from changed to removed
					changed.erase(ad->name);
					removed[ad->name] = ad;
					ad->lastChange = 0;
				}
			}
			replyIter++;
		}
		myself->_pendingBrowseReplies.clear();

		// notify listeners about changes -> serviceResolveRef, addrInfoReply will do so!
		for(std::map<std::string, MDNSAdvertisement*>::iterator changeIter = changed.begin();
		        changeIter != changed.end();
		        changeIter++) {
			MDNSAdvertisement* changedAd = changeIter->second;
			if (changedAd->lastChange & Discovery::IFACE_REMOVED) {
				// addrInfoReply will notify for added interfaces
				assert(myself->_queryClients.find(changedAd->domain) != myself->_queryClients.end());

				UM_LOG_INFO("browseReply: %s/%s of type %s was changed", changedAd->name.c_str(), changedAd->domain.c_str(), changedAd->regType.c_str());

				std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[changedAd->domain].find(changedAd->regType);
				NativeBonjourQuery& nativeQuery = queryIter->second;
				for (std::set<MDNSQuery*>::iterator listIter = nativeQuery.queries.begin();
				        listIter != nativeQuery.queries.end();
				        listIter++) {
					(*listIter)->rs->change(changedAd, toStr(myself), changedAd->lastChange);
				}
			}
			changedAd->lastChange = 0;
		}

		// notify listeners about removals
		for(std::map<std::string, MDNSAdvertisement*>::iterator removeIter = removed.begin();
		        removeIter != removed.end();
		        removeIter++) {
			MDNSAdvertisement* removedAd = removeIter->second;
			assert(myself->_queryClients.find(removedAd->domain) != myself->_queryClients.end());

			std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[removedAd->domain].find(removedAd->regType);
			UM_LOG_INFO("browseReply: %s/%s of type %s was removed", removedAd->name.c_str(), removedAd->domain.c_str(), removedAd->regType.c_str());

			for (std::set<MDNSQuery*>::iterator listIter = queryIter->second.queries.begin();
			        listIter != queryIter->second.queries.end();
			        listIter++) {

				(*listIter)->rs->remove(removedAd, toStr(myself));
				assert(queryIter->second.remoteAds.find(removedAd->name) != queryIter->second.remoteAds.end());
			}

			// stop resolving service
			assert(myself->_remoteAds.find(removedAd) != myself->_remoteAds.end());
			std::map<uint32_t, DNSServiceRef>::iterator svcRefIter;

			svcRefIter = myself->_remoteAds[removedAd].serviceResolverOnIface.begin();
			while(svcRefIter != myself->_remoteAds[removedAd].serviceResolverOnIface.end()) {
#ifdef NON_SHARED_FDS
				myself->_activeFDs.erase(DNSServiceRefSockFD(svcRefIter->second));
#endif
				DNSServiceRefDeallocate(svcRefIter->second);
				svcRefIter++;
			}
			svcRefIter = myself->_remoteAds[removedAd].serviceGetAddrInfoOnIface.begin();
			while(svcRefIter != myself->_remoteAds[removedAd].serviceGetAddrInfoOnIface.end()) {
#ifdef NON_SHARED_FDS
				myself->_activeFDs.erase(DNSServiceRefSockFD(svcRefIter->second));
#endif
				DNSServiceRefDeallocate(svcRefIter->second);
				svcRefIter++;
			}
			myself->_remoteAds.erase(queryIter->second.remoteAds[removedAd->name]);

			// get rid of vanished MDNSAd
			queryIter->second.remoteAds.erase(removeIter->first);
			delete removedAd;
		}
	}
}

/**
 * We found the host for one of the services we browsed for,
 * resolve the hostname to its ip address.
 */
void DNSSD_API BonjourDiscoverer::serviceResolveReply(DNSServiceRef sdref,
        const DNSServiceFlags flags,
        uint32_t interfaceIndex,
        DNSServiceErrorType errorCode,
        const char *fullname,   // full service domain name: <servicename>.<protocol>.<domain>
        const char *hosttarget, // target hostname of the machine providing the service
        uint16_t opaqueport,    // port in network byte order
        uint16_t txtLen,        // length of the txt record
        const unsigned char *txtRecord, // primary txt record
        void *context           // address of node
                                                     ) {

	/* DNSServiceResolve()
	 *
	 * Resolve a service name discovered via DNSServiceBrowse() to a target host name, port number, and
	 * txt record.
	 *
	 * Note: Applications should NOT use DNSServiceResolve() solely for txt record monitoring - use
	 * DNSServiceQueryRecord() instead, as it is more efficient for this task.
	 *
	 * Note: When the desired results have been returned, the client MUST terminate the resolve by calling
	 * DNSServiceRefDeallocate().
	 *
	 * Note: DNSServiceResolve() behaves correctly for typical services that have a single SRV record
	 * and a single TXT record. To resolve non-standard services with multiple SRV or TXT records,
	 * DNSServiceQueryRecord() should be used.
	 *
	 * DNSServiceResolveReply Callback Parameters:
	 *
	 * sdRef:           The DNSServiceRef initialized by DNSServiceResolve().
	 *
	 * flags:           Possible values: kDNSServiceFlagsMoreComing
	 *
	 * interfaceIndex:  The interface on which the service was resolved.
	 *
	 * errorCode:       Will be kDNSServiceErr_NoError (0) on success, otherwise will
	 *                  indicate the failure that occurred. Other parameters are undefined if
	 *                  the errorCode is nonzero.
	 *
	 * fullname:        The full service domain name, in the form <servicename>.<protocol>.<domain>.
	 *                  (This name is escaped following standard DNS rules, making it suitable for
	 *                  passing to standard system DNS APIs such as res_query(), or to the
	 *                  special-purpose functions included in this API that take fullname parameters.
	 *                  See "Notes on DNS Name Escaping" earlier in this file for more details.)
	 *
	 * hosttarget:      The target hostname of the machine providing the service. This name can
	 *                  be passed to functions like gethostbyname() to identify the host's IP address.
	 *
	 * port:            The port, in network byte order, on which connections are accepted for this service.
	 *
	 * txtLen:          The length of the txt record, in bytes.
	 *
	 * txtRecord:       The service's primary txt record, in standard txt record format.
	 *
	 * context:         The context pointer that was passed to the callout.
	 *
	 * NOTE: In earlier versions of this header file, the txtRecord parameter was declared "const char *"
	 * This is incorrect, since it contains length bytes which are values in the range 0 to 255, not -128 to +127.
	 * Depending on your compiler settings, this change may cause signed/unsigned mismatch warnings.
	 * These should be fixed by updating your own callback function definition to match the corrected
	 * function signature using "const unsigned char *txtRecord". Making this change may also fix inadvertent
	 * bugs in your callback function, where it could have incorrectly interpreted a length byte with value 250
	 * as being -6 instead, with various bad consequences ranging from incorrect operation to software crashes.
	 * If you need to maintain portable code that will compile cleanly with both the old and new versions of
	 * this header file, you should update your callback function definition to use the correct unsigned value,
	 * and then in the place where you pass your callback function to DNSServiceResolve(), use a cast to eliminate
	 * the compiler warning, e.g.:
	 *   DNSServiceResolve(sd, flags, index, name, regtype, domain, (DNSServiceResolveReply)MyCallback, context);
	 * This will ensure that your code compiles cleanly without warnings (and more importantly, works correctly)
	 * with both the old header and with the new corrected version.
	 *
	 */

	SharedPtr<BonjourDiscoverer> myself = getInstance();
	RScopeLock lock(myself->_mutex);

	UM_LOG_DEBUG("serviceResolveReply: info on node %s at %p on %s:%d at if %d",
	             fullname,
	             context,
	             hosttarget,
	             ntohs(opaqueport),
	             interfaceIndex);

	MDNSAdvertisement* ad = (MDNSAdvertisement*)context;

	// deallocate service resolver in any case
	assert(myself->_remoteAds[ad].serviceResolverOnIface.find(interfaceIndex) != myself->_remoteAds[ad].serviceResolverOnIface.end());
	assert(myself->_remoteAds[ad].serviceResolverOnIface[interfaceIndex] == sdref);
#ifdef NON_SHARED_FDS
	myself->_activeFDs.erase(DNSServiceRefSockFD(sdref));
#endif
	DNSServiceRefDeallocate(myself->_remoteAds[ad].serviceResolverOnIface[interfaceIndex]);

	myself->_remoteAds[ad].serviceResolverOnIface.erase(interfaceIndex);

	if (errorCode != 0) {
		UM_LOG_ERR("serviceResolveReply called with error: %s", errCodeToString(errorCode).c_str());
		return;
	}

	// do we still care about this node?
	if (myself->_remoteAds.find((MDNSAdvertisement*)context) == myself->_remoteAds.end()) {
		UM_LOG_WARN("serviceResolveReply called for node %p already gone", context);
		return;
	}

	ad->port = ntohs(opaqueport);
	ad->host = hosttarget;

	if (ad->interfaces.find(interfaceIndex) == ad->interfaces.end()) {
		// was removed already
		UM_LOG_WARN("serviceResolveReply called for node %p on interface %i which is gone", context, interfaceIndex);

		return;
	}

	size_t txtOffset = 0;
	while (txtOffset < txtLen) {
		uint8_t length = txtRecord[txtOffset++];
		if (length > 0) {
			ad->txtRecord.insert(std::string(txtRecord[txtOffset], length));
			txtOffset += length;
		}
	}

	DNSServiceRef addrInfoRef = myself->_mainDNSHandle;

	int err = DNSServiceGetAddrInfo(&addrInfoRef,
	                                SHARE_SERVICE_FLAG,    // kDNSServiceFlagsForceMulticast, kDNSServiceFlagsLongLivedQuery, kDNSServiceFlagsReturnIntermediates
	                                interfaceIndex,
	                                kDNSServiceProtocol_IPv4,           // 0, kDNSServiceProtocol_IPv4, kDNSServiceProtocol_IPv6, 0
	                                hosttarget,
	                                addrInfoReply,
	                                (void*)ad                           // address of node
	                               );
	if (err != kDNSServiceErr_NoError) {
		UM_LOG_ERR("DNSServiceGetAddrInfo returned with error: %s", errCodeToString(errorCode).c_str());
		return;
	} else {
		// remember service ref of address resolver
		assert(myself->_remoteAds[ad].serviceGetAddrInfoOnIface.find(interfaceIndex) == myself->_remoteAds[ad].serviceGetAddrInfoOnIface.end());
		myself->_remoteAds[ad].serviceGetAddrInfoOnIface[interfaceIndex] = addrInfoRef;
#ifdef NON_SHARED_FDS
		myself->_activeFDs[DNSServiceRefSockFD(addrInfoRef)] = addrInfoRef;
#endif

	}
}

/**
 * We resolved the IP address for one of the nodes received in serviceResolveReply.
 */
void DNSSD_API BonjourDiscoverer::addrInfoReply(
    DNSServiceRef sdRef_,
    DNSServiceFlags flags_,
    uint32_t interfaceIndex_,
    DNSServiceErrorType errorCode_,
    const char *hostname_,
    const struct sockaddr *address_,
    uint32_t ttl_,
    void *context_  // address of node
) {

	/* DNSServiceGetAddrInfo
	 *
	 * Queries for the IP address of a hostname by using either Multicast or Unicast DNS.
	 *
	 * DNSServiceGetAddrInfoReply() parameters:
	 *
	 * sdRef:           The DNSServiceRef initialized by DNSServiceGetAddrInfo().
	 *
	 * flags:           Possible values are kDNSServiceFlagsMoreComing and
	 *                  kDNSServiceFlagsAdd.
	 *
	 * interfaceIndex:  The interface to which the answers pertain.
	 *
	 * errorCode:       Will be kDNSServiceErr_NoError on success, otherwise will
	 *                  indicate the failure that occurred.  Other parameters are
	 *                  undefined if errorCode is nonzero.
	 *
	 * hostname:        The fully qualified domain name of the host to be queried for.
	 *
	 * address:         IPv4 or IPv6 address.
	 *
	 * ttl:             If the client wishes to cache the result for performance reasons,
	 *                  the TTL indicates how long the client may legitimately hold onto
	 *                  this result, in seconds. After the TTL expires, the client should
	 *                  consider the result no longer valid, and if it requires this data
	 *                  again, it should be re-fetched with a new query. Of course, this
	 *                  only applies to clients that cancel the asynchronous operation when
	 *                  they get a result. Clients that leave the asynchronous operation
	 *                  running can safely assume that the data remains valid until they
	 *                  get another callback telling them otherwise.
	 *
	 * context:         The context pointer that was passed to the callout.
	 *
	 */

	SharedPtr<BonjourDiscoverer> myself = getInstance();
	RScopeLock lock(myself->_mutex);

	UM_LOG_DEBUG("addrInfoReply: %s with ttl %d for %p on %d", hostname_, ttl_, context_, interfaceIndex_);

	if (errorCode_ != 0) {
		UM_LOG_ERR("addrInfoReply called with error: %s", errCodeToString(errorCode_).c_str());
		return;
	}

	if(~flags_ & kDNSServiceFlagsAdd) {
		UM_LOG_WARN("Ignoring addrInfoReply for removed address, relying on browseReply");
		return;
	}

	// do we still care about this node?
	if (myself->_remoteAds.find((MDNSAdvertisement*)context_) == myself->_remoteAds.end()) {
		UM_LOG_WARN("addrInfoReply called for node %p already gone", context_);
		return;
	}

	MDNSAdvertisement* ad = (MDNSAdvertisement*)context_;
	if (ad->interfaces.find(interfaceIndex_) == ad->interfaces.end()) {
		UM_LOG_WARN("addrInfoReply called for node %p on interface already gone", context_);
		return;
	}

	NativeBonjourAddrInfoReply reply;
	reply.context = context_;
	reply.errorCode = errorCode_;
	reply.flags = flags_;
	reply.hostname = hostname_;
	reply.interfaceIndex = interfaceIndex_;
	reply.ttl = ttl_;

	// get ip address from struct
	char* addr = NULL;
	if (address_ && address_->sa_family == AF_INET) {
		const unsigned char *b = (const unsigned char *) &((struct sockaddr_in *)address_)->sin_addr;
		asprintf(&addr, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
		reply.ipv4 = addr;
		free(addr);
	} else if (address_ && address_->sa_family == AF_INET6) {
		const struct sockaddr_in6 *s6 = (const struct sockaddr_in6 *)address_;
		const unsigned char *b = (const unsigned char*)&s6->sin6_addr;
		asprintf(&addr, "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
		         b[0x0], b[0x1], b[0x2], b[0x3], b[0x4], b[0x5], b[0x6], b[0x7],
		         b[0x8], b[0x9], b[0xA], b[0xB], b[0xC], b[0xD], b[0xE], b[0xF]);
		reply.ipv6 = addr;
		free(addr);
	}
	myself->_pendingAddrInfoReplies.push_back(reply);

	// there are no more pending invocations for now, process what we got
	if (~flags_ & kDNSServiceFlagsMoreComing) {
		std::map<std::string, MDNSAdvertisement*> added;   // new service appeared
		std::map<std::string, MDNSAdvertisement*> changed; // some interface added or vanished

		// we queued a list of mdns replies, simplify first
		std::list<NativeBonjourAddrInfoReply>::iterator replyIter = myself->_pendingAddrInfoReplies.begin();
		while(replyIter != myself->_pendingAddrInfoReplies.end()) {

			NativeBonjourAddrInfoReply& reply = *replyIter;
			MDNSAdvertisement* ad = (MDNSAdvertisement*)reply.context;
			uint32_t iface = reply.interfaceIndex;

			// deallocate address resolver
			if (myself->_remoteAds[ad].serviceGetAddrInfoOnIface.find(iface) != myself->_remoteAds[ad].serviceGetAddrInfoOnIface.end()) {
				// make sure ther is no service resolver around
				assert(myself->_remoteAds[ad].serviceResolverOnIface.find(iface) == myself->_remoteAds[ad].serviceResolverOnIface.end());
#ifdef NON_SHARED_FDS
				myself->_activeFDs.erase(DNSServiceRefSockFD(myself->_remoteAds[ad].serviceGetAddrInfoOnIface[iface]));
#endif
				DNSServiceRefDeallocate(myself->_remoteAds[ad].serviceGetAddrInfoOnIface[iface]);
				myself->_remoteAds[ad].serviceGetAddrInfoOnIface.erase(iface);
			}

			if (myself->_queryClients.find(ad->domain) == myself->_queryClients.end()) {
				UM_LOG_ERR("Ignoring addrInfoReply for domain no longer watched: %s", ad->domain.c_str());
				return;
			}

			std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[ad->domain].find(ad->regType);
			if (queryIter == myself->_queryClients[ad->domain].end()) {
				UM_LOG_ERR("Ignoring addrInfoReply for service type no longer watched: %s", ad->regType.c_str());
				return;
			}

			// is this a new node?
			if (ad->ipv4.size() == 0 && ad->ipv6.size() == 0) {
				added[ad->name] = ad;
			} else if (added.find(ad->name) == added.end()) {
				changed[ad->name] = ad;
				ad->lastChange |= Discovery::IFACE_ADDED;
			}

			// copy over ip addresses
			if (reply.ipv4.size() > 0)
				ad->ipv4[iface] = reply.ipv4;
			if (reply.ipv6.size() > 0)
				ad->ipv6[iface] = reply.ipv6;

			replyIter++;
		}
		myself->_pendingAddrInfoReplies.clear();

		//myself->dumpQueries();

//			UM_LOG_INFO("addrInfoReply: %d added, %d changed", added.size(), changed.size());

		// notify listeners about changes
		for(std::map<std::string, MDNSAdvertisement*>::iterator changeIter = changed.begin();
		        changeIter != changed.end();
		        changeIter++) {
			MDNSAdvertisement* changee = changeIter->second;
			assert(myself->_queryClients.find(changee->domain) != myself->_queryClients.end());
			std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[changee->domain].find(changee->regType);
			for (std::set<MDNSQuery*>::iterator listIter = queryIter->second.queries.begin();
			        listIter != queryIter->second.queries.end();
			        listIter++) {
				UM_LOG_DEBUG("addrInfoReply: %s/%s of type %s was changed", changee->name.c_str(), changee->domain.c_str(), changee->regType.c_str());
				(*listIter)->rs->change(changee, toStr(myself), changee->lastChange);
			}
			changee->lastChange = 0;
		}

		// notify listeners about aditions
		for(std::map<std::string, MDNSAdvertisement*>::iterator addIter = added.begin();
		        addIter != added.end();
		        addIter++) {
			MDNSAdvertisement* addee = addIter->second;
			assert(myself->_queryClients.find(addee->domain) != myself->_queryClients.end());
			std::map<std::string, NativeBonjourQuery>::iterator queryIter = myself->_queryClients[addee->domain].find(addee->regType);
			for (std::set<MDNSQuery*>::iterator listIter = queryIter->second.queries.begin();
			        listIter != queryIter->second.queries.end();
			        listIter++) {
				UM_LOG_DEBUG("addrInfoReply:%s/%s of type %s was added at ", addee->name.c_str(), addee->domain.c_str(), addee->regType.c_str());
				(*listIter)->rs->add(addee, toStr(myself));
			}
		}

	}
}

/**
 * Bonjour callback for registering a node.
 * @param sdRef The bonjour handle.
 * @param flags Only kDNSServiceFlagsAdd or not.
 * @param errorCode The error if one occured or kDNSServiceErr_NoError.
 * @param name The actual name the node succeeded to register with.
 * @param regType At the moment this ought to be _mundo._tcp only.
 * @param domain At the moment this ought to be .local only.
 * @param context The address of the node we tried to register.
 */
void DNSSD_API BonjourDiscoverer::registerReply(
    DNSServiceRef            sdRef,
    DNSServiceFlags          flags,
    DNSServiceErrorType      errorCode,
    const char               *name,
    const char               *regtype,
    const char               *domain,
    void                     *context // address of node
) {

	UM_LOG_DEBUG("registerReply: %s %s/%s as %s", (flags & kDNSServiceFlagsAdd ? "Registered" : "Unregistered"), SHORT_UUID(std::string(name)).c_str(), domain, regtype);
	if (errorCode == kDNSServiceErr_NameConflict)
		UM_LOG_WARN("Name conflict with UUIDs?!");
}

void BonjourDiscoverer::dumpQueries() {
	std::map<std::string, std::map<std::string, NativeBonjourQuery> >::iterator domainIter = _queryClients.begin();
	while(domainIter != _queryClients.end()) {
		std::cout << domainIter->first << std::endl;
		std::map<std::string, NativeBonjourQuery>::iterator typeIter = domainIter->second.begin();
		while(typeIter != domainIter->second.end()) {
			std::cout << "\t" << typeIter->first << std::endl;
			std::map<std::string, MDNSAdvertisement*>::iterator adIter = typeIter->second.remoteAds.begin();
			while(adIter != typeIter->second.remoteAds.end()) {
				std::cout << "\t\t" << "domain: " << adIter->second->domain;
				std::cout << "" << ", host: " << adIter->second->host;
				std::cout << "" << ", inProc: " << adIter->second->isInProcess;
				std::cout << "" << ", remote: " << adIter->second->isRemote;
				std::cout << "" << ", name: " << adIter->second->name;
				std::cout << "" << ", port: " << adIter->second->port;
				std::cout << "" << ", regType: " << adIter->second->regType << std::endl;
				std::cout << "" << ", txtRecord: ";
				std::set<std::string>::iterator txtIter = adIter->second->txtRecord.begin();
				while(txtIter != adIter->second->txtRecord.end()) {
					std::cout << " / " << *txtIter;
					txtIter++;
				}
				std::cout << "" << "interfaces:";
				std::set<uint32_t>::iterator ifIter = adIter->second->interfaces.begin();
				while(ifIter != adIter->second->interfaces.end()) {
					std::cout << " " << *ifIter;
					ifIter++;
				}

				std::cout << ", ipv4: ";
				std::map<uint32_t, std::string>::iterator ipv4Iter = adIter->second->ipv4.begin();
				while(ipv4Iter != adIter->second->ipv4.end()) {
					std::cout << ", " << ipv4Iter->first << ": " << ipv4Iter->second;
					ipv4Iter++;
				}
				std::cout << ", ipv6: ";
				std::map<uint32_t, std::string>::iterator ipv6Iter = adIter->second->ipv6.begin();
				while(ipv6Iter != adIter->second->ipv6.end()) {
					std::cout << ", " << ipv6Iter->first << ": " << ipv6Iter->second;
					ipv6Iter++;
				}
				adIter++;
				std::cout << std::endl;
			}
			typeIter++;
		}
		domainIter++;
		std::cout << std::endl;
		std::cout << std::endl;
	}
}

const std::string BonjourDiscoverer::errCodeToString(DNSServiceErrorType errType) {
	switch (errType) {
	case kDNSServiceErr_NoError:
		return "kDNSServiceErr_NoError";
	case kDNSServiceErr_Unknown:
		return "kDNSServiceErr_Unknown";
	case kDNSServiceErr_NoSuchName:
		return "kDNSServiceErr_NoSuchName";
	case kDNSServiceErr_NoMemory:
		return "kDNSServiceErr_NoMemory";
	case kDNSServiceErr_BadParam:
		return "kDNSServiceErr_BadParam";
	case kDNSServiceErr_BadReference:
		return "kDNSServiceErr_BadReference";
	case kDNSServiceErr_BadState:
		return "kDNSServiceErr_BadState";
	case kDNSServiceErr_BadFlags:
		return "kDNSServiceErr_BadFlags";
	case kDNSServiceErr_Unsupported:
		return "kDNSServiceErr_Unsupported";
	case kDNSServiceErr_NotInitialized:
		return "kDNSServiceErr_NotInitialized";
	case kDNSServiceErr_AlreadyRegistered:
		return "kDNSServiceErr_AlreadyRegistered";
	case kDNSServiceErr_NameConflict:
		return "kDNSServiceErr_NameConflict";
	case kDNSServiceErr_Invalid:
		return "kDNSServiceErr_Invalid";
	case kDNSServiceErr_Firewall:
		return "kDNSServiceErr_Firewall";
	case kDNSServiceErr_Incompatible:
		return "kDNSServiceErr_Incompatible (client library incompatible with daemon)";
	case kDNSServiceErr_BadInterfaceIndex:
		return "kDNSServiceErr_BadInterfaceIndex";
	case kDNSServiceErr_Refused:
		return "kDNSServiceErr_Refused";
	case kDNSServiceErr_NoSuchRecord:
		return "kDNSServiceErr_NoSuchRecord";
	case kDNSServiceErr_NoAuth:
		return "kDNSServiceErr_NoAuth";
	case kDNSServiceErr_NoSuchKey:
		return "kDNSServiceErr_NoSuchKey";
	case kDNSServiceErr_NATTraversal:
		return "kDNSServiceErr_NATTraversal";
	case kDNSServiceErr_DoubleNAT:
		return "kDNSServiceErr_DoubleNAT";
	case kDNSServiceErr_BadTime:
		return "kDNSServiceErr_BadTime";
	case kDNSServiceErr_BadSig:
		return "kDNSServiceErr_BadSig";
	case kDNSServiceErr_BadKey:
		return "kDNSServiceErr_BadKey";
	case kDNSServiceErr_Transient:
		return "kDNSServiceErr_Transient";
	case kDNSServiceErr_ServiceNotRunning:
		return "kDNSServiceErr_ServiceNotRunning (Background daemon not running)";
	case kDNSServiceErr_NATPortMappingUnsupported:
		return "kDNSServiceErr_NATPortMappingUnsupported (NAT doesn't support NAT-PMP or UPnP)";
	case kDNSServiceErr_NATPortMappingDisabled:
		return "kDNSServiceErr_NATPortMappingDisabled (NAT supports NAT-PMP or UPnP but it's disabled by the administrator)";
	case kDNSServiceErr_NoRouter:
		return "kDNSServiceErr_NoRouter (No router currently configured (probably no network connectivity))";
	case kDNSServiceErr_PollingMode:
		return "kDNSServiceErr_PollingMode";
#ifdef BONJOUR_HAS_KDNSSERVICEERR_TIMEOUT
	case kDNSServiceErr_Timeout:
		return "kDNSServiceErr_Timeout";
#endif
	}
	return "unknown error";
}


}
