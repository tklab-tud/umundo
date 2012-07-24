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

#include "umundo/config.h"

#ifdef WIN32
#include <WS2tcpip.h>
#endif

#include "umundo/discovery/bonjour/BonjourNodeDiscovery.h"

#include <errno.h>

#if (defined UNIX || defined IOS || defined ANDROID)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h> // gethostname
#include <stdio.h>
#include <string.h>
#endif

#include "umundo/connection/Node.h"
#include "umundo/discovery/NodeQuery.h"
#include "umundo/discovery/bonjour/BonjourNodeStub.h"

#ifdef DISC_BONJOUR_EMBED
extern "C" {
	/// See mDNSEmbedded.c
	int embedded_mDNSInit();
	void embedded_mDNSExit();
	int embedded_mDNSmainLoop(timeval);
}
#endif

namespace umundo {

BonjourNodeDiscovery::BonjourNodeDiscovery() {
}

shared_ptr<Implementation> BonjourNodeDiscovery::create(void*) {
	return getInstance();
}

void BonjourNodeDiscovery::destroy() {
	// do nothing?
}

void BonjourNodeDiscovery::init(shared_ptr<Configuration>) {
	// do nothing?
}

shared_ptr<BonjourNodeDiscovery> BonjourNodeDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		_instance = shared_ptr<BonjourNodeDiscovery>(new BonjourNodeDiscovery());
#ifdef DISC_BONJOUR_EMBED
		LOG_DEBUG("Initializing embedded mDNS server");
		embedded_mDNSInit();
#endif
		_instance->start();
	}
	return _instance;
}
shared_ptr<BonjourNodeDiscovery> BonjourNodeDiscovery::_instance;

BonjourNodeDiscovery::~BonjourNodeDiscovery() {
	UMUNDO_LOCK(_mutex); // we had some segfaults in validateState from other threads?
	stop();
#ifndef DISC_BONJOUR_EMBED
//	join(); // we have deadlock in embedded?
#endif
#ifdef DISC_BONJOUR_EMBED
	// notify every other host that we are about to vanish
	embedded_mDNSExit();
#endif
}

/**
 * Unregister all nodes when suspending.
 *
 * When we are suspended, we unadvertise all registered nodes and save them as
 * _suspendedNodes to add them when resuming.
 */
void BonjourNodeDiscovery::suspend() {
	UMUNDO_LOCK(_mutex);
	if (_isSuspended) {
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	_isSuspended = true;

	// remove all nodes from discovery
	_suspendedNodes = _localNodes;
	map<intptr_t, shared_ptr<NodeImpl> >::iterator nodeIter = _localNodes.begin();
	while(nodeIter != _localNodes.end()) {
		shared_ptr<NodeImpl> node = nodeIter->second;
		nodeIter++;
		remove(node);
	}
	UMUNDO_UNLOCK(_mutex);
}

/**
 * Re-register nodes previously suspended.
 *
 * We remembered all our nodes in _suspendedNodes when this object was suspended.
 * By explicitly calling resume on the nodes we add again, we make sure they are
 * reinitialized before advertising them.
 */
void BonjourNodeDiscovery::resume() {
	UMUNDO_LOCK(_mutex);
	if (!_isSuspended) {
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	_isSuspended = false;

	map<intptr_t, shared_ptr<NodeImpl> >::iterator nodeIter = _suspendedNodes.begin();
	while(nodeIter != _suspendedNodes.end()) {
		shared_ptr<NodeImpl> node = nodeIter->second;
		nodeIter++;
		// make sure advertised nodes are initialized
		node->resume();
		add(node);
	}
	_suspendedNodes.clear();
	UMUNDO_UNLOCK(_mutex);
}

void BonjourNodeDiscovery::run() {
	struct timeval tv;

#ifdef DISC_BONJOUR_EMBED
	while(isStarted()) {
		UMUNDO_LOCK(_mutex);
		tv.tv_sec  = BONJOUR_REPOLL_SEC;
		tv.tv_usec = BONJOUR_REPOLL_USEC;
		embedded_mDNSmainLoop(tv);
		UMUNDO_UNLOCK(_mutex);
		// give other threads a chance to react before locking again
#ifdef WIN32
		Thread::sleepMs(100);
#else
		Thread::yield();
#endif
	}
#else

	fd_set readfds;
	int nfds = -1;

	LOG_DEBUG("Beginning select");
	while(isStarted()) {
		nfds = 0;
		FD_ZERO(&readfds);
		// tv structure gets modified we have to reset each time
		tv.tv_sec  = BONJOUR_REPOLL_SEC;
		tv.tv_usec = BONJOUR_REPOLL_USEC;

		UMUNDO_LOCK(_mutex);
		// initialize file desriptor set for select
		std::map<int, DNSServiceRef>::const_iterator cIt;
		for (cIt = _activeFDs.begin(); cIt != _activeFDs.end(); cIt++) {
			if (cIt->first > nfds)
				nfds = cIt->first;
			FD_SET(cIt->first, &readfds);
		}
		nfds++;

		int result = select(nfds, &readfds, (fd_set*)NULL, (fd_set*)NULL, &tv);

		// dispatch over result, make sure to unlock mutex everywhere
		if (result > 0) {
			std::map<int, DNSServiceRef>::iterator it;

			it = _activeFDs.begin();
			while(it != _activeFDs.end()) {
				DNSServiceRef sdref = it->second;
				int sockFD = it->first;
				it++;
				if (FD_ISSET(sockFD, &readfds)) {
					FD_CLR(sockFD, &readfds);
					DNSServiceProcessResult(sdref);
					it = _activeFDs.begin();
				}
			}
			UMUNDO_UNLOCK(_mutex);
		} else if (result == 0) {
			// timeout as no socket is selectable, just retry
			UMUNDO_UNLOCK(_mutex);
		} else {
			UMUNDO_UNLOCK(_mutex);
			if (errno != 0)
				LOG_WARN("select failed %s", strerror(errno));
			Thread::sleepMs(300);
		}
	}
#endif
}

/**
 * Add a node to be discoverable by other nodes.
 */
void BonjourNodeDiscovery::add(shared_ptr<NodeImpl> node) {
	LOG_INFO("Adding node %s", SHORT_UUID(node->getUUID()).c_str());
	assert(node->getUUID().length() > 0);

	UMUNDO_LOCK(_mutex);

	DNSServiceErrorType err;
	DNSServiceRef registerClient;
	intptr_t address = (intptr_t)(node.get());

	if (_localNodes.find(address) != _localNodes.end()) {
		// there has to be a register client if we know the node
		assert(_registerClients.find(address) != _registerClients.end());
		LOG_WARN("Ignoring addition of node already added to discovery");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}

	// keep in mind: we prevent the registration of the node with several BonjourNodeDiscoveries
	assert(_registerClients.find(address) == _registerClients.end());

	const char* name = (node->getUUID().length() ? node->getUUID().c_str() : NULL);
	const char* transport = (node->getTransport().length() ? node->getTransport().c_str() : "tcp");
	const char* host = (node->getHost().length() ? node->getHost().c_str() : NULL);
	uint16_t port = (node->getPort() > 0 ? htons(node->getPort()) : 0);
	const char* txtRecord = "";
	uint16_t txtLength = 0;

	char* regtype;
	asprintf(&regtype, "_mundo._%s", transport);

	char* domain;
	if (strstr(node->getDomain().c_str(), ".") != NULL) {
		asprintf(&domain, "%s", node->getDomain().c_str());
	} else if (node->getDomain().length() > 0) {
		asprintf(&domain, "%s.local.", node->getDomain().c_str());
	} else {
		asprintf(&domain, "local.");
	}

	LOG_DEBUG("Trying to register: %s.%s as %s",
	          (name == NULL ? "null" : strndup(name, 8)),
	          (domain == NULL ? "null" : domain),
	          regtype);

#ifdef DISC_BONJOUR_EMBED
	LOG_DEBUG("Calling DNSServiceRegister");
#endif
	err = DNSServiceRegister(
	          &registerClient,         // uninitialized DNSServiceRef
	          0,                               // renaming behavior on name conflict (kDNSServiceFlagsNoAutoRename)
	          kDNSServiceInterfaceIndexAny,    // If non-zero, specifies the interface, defaults to all interfaces
	          name,                            // If non-NULL, specifies the service name, defaults to computer name
	          regtype,                         // service type followed by the protocol
	          domain,                          // If non-NULL, specifies the domain, defaults to default domain
	          host,                            // If non-NULL, specifies the SRV target host name
	          port,                            // port number, defaults to name-reserving/non-discoverable
	          txtLength,                       // length of the txtRecord, in bytes
	          txtRecord,                       // TXT record rdata: <length byte> <data> <length byte> <data> ...
	          registerReply,                   // called when the registration completes
	          (void*)address                   // context pointer which is passed to the callback
	      );
#ifdef DISC_BONJOUR_EMBED
	LOG_DEBUG("DNSServiceRegister returned");
#endif

	free(regtype);
	free(domain);

	if(registerClient && err == 0) {
		// everything is fine, add FD to query set, remember query and node
		int sockFD = DNSServiceRefSockFD(registerClient); // this is a noop in embedded
#ifndef DISC_BONJOUR_EMBED
		assert(getInstance()->_activeFDs.find(sockFD) == getInstance()->_activeFDs.end());
#endif
		assert(getInstance()->_registerClients.find(address) == getInstance()->_registerClients.end());
		assert(getInstance()->_localNodes.find(address) == getInstance()->_localNodes.end());
		getInstance()->_activeFDs[sockFD] = registerClient;
		getInstance()->_registerClients[address] = registerClient;
		getInstance()->_localNodes[address] = node;
	} else {
		LOG_ERR("DNSServiceRegister returned error %d", err);
	}
	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}


void BonjourNodeDiscovery::remove(shared_ptr<NodeImpl> node) {
	LOG_INFO("Removing node %s", SHORT_UUID(node->getUUID()).c_str());

	UMUNDO_LOCK(_mutex);

	intptr_t address = (intptr_t)(node.get());
	if (_localNodes.find(address) == _localNodes.end()) {
		LOG_WARN("Ignoring removal of unregistered node from discovery");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}

	assert(_registerClients.find(address) != _registerClients.end());
#ifndef DISC_BONJOUR_EMBED
	assert(_activeFDs.find(DNSServiceRefSockFD(_registerClients[address])) != _activeFDs.end());
	assert(_activeFDs[DNSServiceRefSockFD(_registerClients[address])] == _registerClients[address]);
	_activeFDs.erase(DNSServiceRefSockFD(_registerClients[address])); // noop in embedded
#endif
	DNSServiceRefDeallocate(_registerClients[address]);
	_registerClients.erase(address);
	_localNodes.erase(address);

	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}


/**
 * Start to browse for other nodes.
 *
 * If we find a node we will try to resolve its hostname and if we found its hostname,
 * we will resolve its ip address.
 */
void BonjourNodeDiscovery::browse(shared_ptr<NodeQuery> query) {
	LOG_INFO("Adding query %p for nodes in %s", query.get(), query->getDomain().c_str());
	DNSServiceErrorType err;
	DNSServiceRef queryClient = NULL;

	UMUNDO_LOCK(_mutex);
	intptr_t address = (intptr_t)(query.get());

	if (_queries.find(address) != _queries.end()) {
		LOG_WARN("Already browsing for given query");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}
	assert(_queryClients.find(address) == _queryClients.end());

	char* regtype;
	const char* transport = (query->getTransport().length() ? query->getTransport().c_str() : "tcp");
	asprintf(&regtype, "_mundo._%s", transport);

	char* domain;
	if (strstr(query->getDomain().c_str(), ".") != NULL) {
		asprintf(&domain, "%s", query->getDomain().c_str());
	} else if (query->getDomain().length() > 0) {
		asprintf(&domain, "%s.local.", query->getDomain().c_str());
	} else {
		asprintf(&domain, "local.");
	}

#ifdef DISC_BONJOUR_EMBED
	LOG_DEBUG("Calling DNSServiceBrowse");
#endif
	err = DNSServiceBrowse(
	          &queryClient,                  // uninitialized DNSServiceRef
	          0,                             // Currently ignored, reserved for future use
	          kDNSServiceInterfaceIndexAny,  // non-zero, specifies the interface
	          regtype,                       // service type being browsed
	          domain,                        // non-NULL, specifies the domain
	          browseReply,                   // called when a service is found
	          (void*)address
	      );

#ifdef DISC_BONJOUR_EMBED
	LOG_DEBUG("DNSServiceBrowse returned");
#endif

	if(queryClient && err == 0) {
		// query started succesfully, remember
#ifndef DISC_BONJOUR_EMBED
		int sockFD = DNSServiceRefSockFD(queryClient);
		assert(_activeFDs.find(sockFD) == _activeFDs.end());
		_activeFDs[sockFD] = queryClient;
#endif
		assert(_queryClients.find(address) == _queryClients.end());
		assert(_queries.find(address) == _queries.end());
		_queryClients[address] = queryClient;
		_queries[address] = query;
	} else {
		LOG_WARN("DNSServiceBrowse returned error %d", err);
	}

	free(regtype);
	free(domain);
	assert(validateState());

	UMUNDO_UNLOCK(_mutex);
}

void BonjourNodeDiscovery::unbrowse(shared_ptr<NodeQuery> query) {
	LOG_INFO("Removing query %p for nodes in %s", query.get(), query->getDomain().c_str());

	UMUNDO_LOCK(_mutex);
	intptr_t address = (intptr_t)(query.get());

	if (_queries.find(address) == _queries.end()) {
		LOG_WARN("Unbrowsing query that was never added");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}

	assert(_queryClients.find(address) != _queryClients.end());
#ifndef DISC_BONJOUR_EMBED
	assert(_activeFDs.find(DNSServiceRefSockFD(_queryClients[address])) != _activeFDs.end());
	assert(_activeFDs[DNSServiceRefSockFD(_queryClients[address])] == _queryClients[address]);
	_activeFDs.erase(DNSServiceRefSockFD(_queryClients[address])); // remove fd from active set
#endif

	/**
	 * Remove all nodes we found for the query
	 */
	map<intptr_t, shared_ptr<NodeQuery> >::iterator nodeToQueryIter = _nodeToQuery.begin();
	while (nodeToQueryIter != _nodeToQuery.end()) {
		if (nodeToQueryIter->second == query) {
			_nodeToQuery.erase(nodeToQueryIter++);
		} else {
			nodeToQueryIter++;
		}
	}
	map<string, shared_ptr<BonjourNodeStub> >::iterator queryNodeIter;
	for (queryNodeIter = _queryToNodes[query].begin(); queryNodeIter != _queryToNodes[query].end(); queryNodeIter++) {
		// for every node found for the query, remove all bonjour queries
		shared_ptr<BonjourNodeStub> node = queryNodeIter->second;
		getInstance()->forgetRemoteNodesFDs(node);
	}

	_queryToNodes.erase(query);
	_queries.erase(address);

	DNSServiceRefDeallocate(_queryClients[address]);
	_queryClients.erase(address);

	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}

void BonjourNodeDiscovery::forgetRemoteNodesFDs(shared_ptr<BonjourNodeStub> node) {
	UMUNDO_LOCK(_mutex);

	map<string, DNSServiceRef>::iterator serviceResolveIter = node->_serviceResolveClients.begin();
	while (serviceResolveIter != node->_serviceResolveClients.end()) {
#ifndef DISC_BONJOUR_EMBED
		int sockFD = DNSServiceRefSockFD(serviceResolveIter->second);
		assert(_activeFDs.find(sockFD) != _activeFDs.end());
		_activeFDs.erase(sockFD);
#endif
		DNSServiceRefDeallocate(serviceResolveIter->second);
		string domain = serviceResolveIter->first;
		serviceResolveIter++;
		node->_serviceResolveClients.erase(domain);
	}

	map<int, DNSServiceRef>::iterator addrInfoIter = node->_addrInfoClients.begin();
	while(addrInfoIter != node->_addrInfoClients.end()) {
#ifndef DISC_BONJOUR_EMBED
		int sockFD = DNSServiceRefSockFD(addrInfoIter->second);
		assert(_activeFDs.find(sockFD) != _activeFDs.end());
		_activeFDs.erase(sockFD);
#endif
		DNSServiceRefDeallocate(addrInfoIter->second);
		int ifIndex = addrInfoIter->first;
		addrInfoIter++;
		node->_addrInfoClients.erase(ifIndex);
	}

	UMUNDO_UNLOCK(_mutex);
}

/**
 * There has been an answer to one of our queries.
 *
 * Gather all additions, changes and removals and start service resolving.
 */
void DNSSD_API BonjourNodeDiscovery::browseReply(
    DNSServiceRef sdref,
    const DNSServiceFlags flags,
    uint32_t ifIndex,
    DNSServiceErrorType errorCode,
    const char *replyName,
    const char *replyType,
    const char *replyDomain,
    void *queryAddr) {

	shared_ptr<BonjourNodeDiscovery> myself = getInstance();
	UMUNDO_LOCK(myself->_mutex);

	LOG_DEBUG("browseReply: %p received info on %s/%s as %s at if %d",
	          queryAddr, strndup(replyName, 8), replyDomain, replyType, ifIndex);

	assert(myself->_queries.find((intptr_t)queryAddr) != myself->_queries.end());
	shared_ptr<NodeQuery> query = myself->_queries[(intptr_t)queryAddr];

	// do we already know this node?
	shared_ptr<BonjourNodeStub> node;
	if (getInstance()->_queryToNodes[query].find(replyName) != getInstance()->_queryToNodes[query].end())
		node = boost::static_pointer_cast<BonjourNodeStub>(getInstance()->_queryToNodes[query][replyName]);

	if (flags & kDNSServiceFlagsAdd) {
		// we have a node to add
		if (node.get() == NULL) {
			node = shared_ptr<BonjourNodeStub>(new BonjourNodeStub());
			// remember which query found us
			assert(getInstance()->_nodeToQuery.find((intptr_t)node.get()) == getInstance()->_nodeToQuery.end());
			getInstance()->_nodeToQuery[(intptr_t)node.get()] = query;
			if (strncmp(replyType + strlen(replyType) - 4, "tcp", 3) == 0) {
				node->setTransport("tcp");
			} else if (strncmp(replyType + strlen(replyType) - 4, "udp", 3) == 0) {
				node->setTransport("udp");
			} else {
				LOG_WARN("Unknown transport %s, defaulting to tcp", replyType + strlen(replyType) - 4);
				node->setTransport("tcp");
			}
			node->_uuid = replyName;
			node->_domain = replyDomain;
			node->_regType = replyType;
			node->_isInProcess = false;
			node->_isRemote = true;

			// is this a "in-process" node?
			map<intptr_t, shared_ptr<NodeImpl> >::iterator localNodeIter = getInstance()->_localNodes.begin();
			while(localNodeIter != getInstance()->_localNodes.end()) {
				if (localNodeIter->second->getUUID().compare(replyName) == 0) {
					node->setInProcess(true);
				}
				localNodeIter++;
			}

			// remember node
			assert(getInstance()->_queryToNodes[query].find(replyName) == getInstance()->_queryToNodes[query].end());
			getInstance()->_queryToNodes[query][replyName] = node;
			LOG_INFO("Query %p discovered new node %s", queryAddr, SHORT_UUID(node->getUUID()).c_str());
		}
		// even if we found the node before, add the new information
		node->_domains.insert(replyDomain);
		node->_interfaceIndices.insert(ifIndex);
	} else {
		// remove or change the node or an interface
		assert(node.get() != NULL);
		node->_interfaceIndices.erase(ifIndex);
		node->_interfacesIPv4.erase(ifIndex);
		node->_interfacesIPv6.erase(ifIndex);
		if (node->_interfaceIndices.empty()) {
			// last interface was removed, node vanished
			LOG_INFO("Query %p vanished node %s", queryAddr, SHORT_UUID(node->getUUID()).c_str());
			query->removed(boost::static_pointer_cast<NodeStub>(node));
			getInstance()->forgetRemoteNodesFDs(node);
			assert(getInstance()->_nodeToQuery.find((intptr_t)node.get()) != getInstance()->_nodeToQuery.end());
			getInstance()->_nodeToQuery.erase((intptr_t)node.get());
			getInstance()->_queryToNodes[query].erase(node->getUUID());
		}
	}

	if (!(flags & kDNSServiceFlagsMoreComing)) {
		/**
		* There are no more bonjour notifications waiting, start resolving all new nodes
		* For each node, make sure there is a resolve client for every domain.
		*/
		map<string, shared_ptr<BonjourNodeStub> >::iterator nodeIter;
		for (nodeIter = getInstance()->_queryToNodes[query].begin(); nodeIter != getInstance()->getInstance()->_queryToNodes[query].end(); nodeIter++) {
			string uuid = nodeIter->first;
			shared_ptr<BonjourNodeStub> node = nodeIter->second;

			intptr_t address = (intptr_t)(node.get());
			char* regtype;
			assert(node->getTransport().length() > 0);
			asprintf(&regtype, "_mundo._%s", node->getTransport().c_str());
			const char* name = (node->getUUID().length() ? node->getUUID().c_str() : NULL);

			std::set<string>::iterator domainIter;
			for (domainIter = node->_domains.begin(); domainIter != node->_domains.end(); domainIter++) {
				if (node->_serviceResolveClients.find(*domainIter) != node->_serviceResolveClients.end())
					// we already have a service resolve client for that domain
					continue;

				DNSServiceRef serviceResolveClient;
				/// Resolve service domain name, target hostname, port number and txt record
				int err = DNSServiceResolve(
				              &serviceResolveClient,
				              0,
				              kDNSServiceInterfaceIndexAny,
				              name,
				              regtype,
				              domainIter->c_str(),
				              serviceResolveReply,
				              (void*)address // address of node
				          );

				if(serviceResolveClient && err == kDNSServiceErr_NoError) {
#ifndef DISC_BONJOUR_EMBED
					int sockFD = DNSServiceRefSockFD(serviceResolveClient);
					assert(getInstance()->_activeFDs.find(sockFD) == getInstance()->_activeFDs.end());
					getInstance()->_activeFDs[sockFD] = serviceResolveClient;
#endif
					assert(node->_serviceResolveClients.find(*domainIter) == node->_serviceResolveClients.end());
					node->_serviceResolveClients[*domainIter] = serviceResolveClient;
				} else {
					LOG_WARN("DNSServiceResolve returned error %d", err);
				}
			}

			free(regtype);
		}
	}
	assert(myself->validateState());
	UMUNDO_UNLOCK(myself->_mutex);
}

/**
 * We found the host for one of the services we browsed for,
 * resolve the hostname to its ip address.
 */
void DNSSD_API BonjourNodeDiscovery::serviceResolveReply(
    DNSServiceRef sdref,
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

	UMUNDO_LOCK(getInstance()->_mutex);
	LOG_DEBUG("serviceResolveReply: info on node %s on %s:%d at if %d",
	          strndup(fullname, 8),
	          hosttarget,
	          ntohs(opaqueport),
	          interfaceIndex);
	assert(getInstance()->validateState());

	if(errorCode == kDNSServiceErr_NoError) {
		shared_ptr<NodeQuery> query = getInstance()->_nodeToQuery[(intptr_t)context];
		shared_ptr<BonjourNodeStub> node = getInstance()->_queryToNodes[query][((BonjourNodeStub*)context)->getUUID()];

		node->_fullname = fullname;
		node->_host = hosttarget;

		// is this the local host?
		char hostname[1024];
		hostname[1023] = '\0';
		gethostname(hostname, 1023);

		if (strncmp(hosttarget, hostname, strlen(hostname)) == 0) {
			node->setRemote(false);
		}
		node->_port = ntohs(opaqueport);

		const char* domainStart = strstr(fullname, node->_regType.c_str());
		domainStart += node->_regType.size();
		assert(node->_domain.compare(domainStart) == 0);

		// Now that we know the service, resolve its IP
		DNSServiceRef addrInfoClient;
		int err = DNSServiceGetAddrInfo(
		              &addrInfoClient,
		              kDNSServiceFlagsReturnIntermediates,     // kDNSServiceFlagsForceMulticast, kDNSServiceFlagsLongLivedQuery
		              interfaceIndex,
		              0,                                       // kDNSServiceProtocol_IPv4, kDNSServiceProtocol_IPv6, 0
		              node->_host.c_str(),
		              addrInfoReply,
		              (void*)context                           // address of node
		          );

		if (err == kDNSServiceErr_NoError && addrInfoClient) {
			if (node->_addrInfoClients.find(interfaceIndex) != node->_addrInfoClients.end()) {
				// remove old query
#ifndef DISC_BONJOUR_EMBED
				int sockFD = DNSServiceRefSockFD(node->_addrInfoClients[interfaceIndex]);
				assert(getInstance()->_activeFDs.find(sockFD) != getInstance()->_activeFDs.end());
				getInstance()->_activeFDs.erase(sockFD);
#endif
				DNSServiceRefDeallocate(node->_addrInfoClients[interfaceIndex]);
				node->_addrInfoClients.erase(interfaceIndex);
			}
			assert(node->_addrInfoClients.find(interfaceIndex) == node->_addrInfoClients.end());
			node->_addrInfoClients[interfaceIndex] = addrInfoClient;
#ifndef DISC_BONJOUR_EMBED
			int sockFD = DNSServiceRefSockFD(addrInfoClient);
			assert(getInstance()->_activeFDs.find(sockFD) == getInstance()->_activeFDs.end());
			getInstance()->_activeFDs[sockFD] = addrInfoClient;
#endif
		} else {
			LOG_ERR("DNSServiceGetAddrInfo returned error");
		}

		if (!(flags & kDNSServiceFlagsMoreComing)) {
			// remove the service resolver for this domain
			assert(node->_serviceResolveClients.find(node->_domain) != node->_serviceResolveClients.end());
			assert(node->_serviceResolveClients[node->_domain] == sdref);
#ifndef DISC_BONJOUR_EMBED
			int sockFD = DNSServiceRefSockFD(node->_serviceResolveClients[node->_domain]);
			assert(getInstance()->_activeFDs.find(sockFD) != getInstance()->_activeFDs.end());
			getInstance()->_activeFDs.erase(sockFD);
#endif
			DNSServiceRefDeallocate(node->_serviceResolveClients[node->_domain]);
			node->_serviceResolveClients.erase(node->_domain);
		}
	} else {
		LOG_WARN("serviceResolveReply called with error: %d", errorCode);
	}
	assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
}

/**
 * We resolved the IP address for one of the nodes received in serviceResolveReply.
 */
void DNSSD_API BonjourNodeDiscovery::addrInfoReply(
    DNSServiceRef sdRef,
    DNSServiceFlags flags,
    uint32_t interfaceIndex,
    DNSServiceErrorType errorCode,
    const char *hostname,
    const struct sockaddr *address,
    uint32_t ttl,
    void *context  // address of node
) {

	UMUNDO_LOCK(getInstance()->_mutex);
	if (getInstance()->_nodeToQuery.find((intptr_t)context) == getInstance()->_nodeToQuery.end()) {
		LOG_WARN("Ignoring address information for unknown node %p", context);
		UMUNDO_UNLOCK(getInstance()->_mutex);
		return;
	}

	shared_ptr<NodeQuery> query = getInstance()->_nodeToQuery[(intptr_t)context];
	shared_ptr<BonjourNodeStub> node = getInstance()->_queryToNodes[query][((BonjourNodeStub*)context)->getUUID()];

//  LOG_DEBUG("addrInfoReply: Got info on %s at if %d", hostname, interfaceIndex);

	if (node->_interfaceIndices.find(interfaceIndex) == node->_interfaceIndices.end()) {
		LOG_WARN("Got information on unknown interface index %d", interfaceIndex);
		assert(getInstance()->validateState());
		UMUNDO_UNLOCK(getInstance()->_mutex);
		return;
	}

	if (!address || (address->sa_family != AF_INET && address->sa_family != AF_INET6)) {
		LOG_ERR("Address family not valid or no address given");
		assert(getInstance()->validateState());
		UMUNDO_UNLOCK(getInstance()->_mutex);
		return;
	}

	switch(errorCode) {
	case kDNSServiceErr_NoError: {
		char* addr = NULL;

		if (address && address->sa_family == AF_INET) {
			const unsigned char *b = (const unsigned char *) &((struct sockaddr_in *)address)->sin_addr;
			asprintf(&addr, "%d.%d.%d.%d", b[0], b[1], b[2], b[3]);
			node->_interfacesIPv4[interfaceIndex] = addr;
			LOG_DEBUG("addrInfoReply: %p found %s as %s at if %d IPV4", context, hostname, addr, interfaceIndex);

			free(addr);
		}	else if (address && address->sa_family == AF_INET6) {
			const struct sockaddr_in6 *s6 = (const struct sockaddr_in6 *)address;
			const unsigned char *b = (const unsigned char*)&s6->sin6_addr;

			asprintf(&addr, "%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X:%02X%02X",
			         b[0x0], b[0x1], b[0x2], b[0x3], b[0x4], b[0x5], b[0x6], b[0x7],
			         b[0x8], b[0x9], b[0xA], b[0xB], b[0xC], b[0xD], b[0xE], b[0xF]);
			node->_interfacesIPv6[interfaceIndex] = addr;
			LOG_DEBUG("addrInfoReply: %p found %s as %s at if %d IPV6", context, hostname, addr, interfaceIndex);

			free(addr);
		}
		break;
	}
	case kDNSServiceErr_NoSuchRecord:
		if (address && address->sa_family == AF_INET) {
			LOG_INFO("addrInfoReply: %p found kDNSServiceErr_NoSuchRecord for IPv4 %s at if %d", context, hostname, interfaceIndex);
			node->_interfacesIPv4[interfaceIndex] = "";
		} else if (address->sa_family == AF_INET6) {
			LOG_DEBUG("addrInfoReply: %p found kDNSServiceErr_NoSuchRecord for IPv6 %s at if %d", context, hostname, interfaceIndex);
			node->_interfacesIPv6[interfaceIndex] = "";
		}
		break;
	default:
		LOG_WARN("addrInfoReply error %d for %s at interface %d", errorCode, hostname, interfaceIndex);
		break;
	}
	if (node->_interfacesIPv6.find(interfaceIndex) != node->_interfacesIPv6.end() &&
	        node->_interfacesIPv4.find(interfaceIndex) != node->_interfacesIPv4.end()) {
		// we resolved this iface ipv4 and ipv6 addresses - remove the address resolver for this interface
		assert(node->_addrInfoClients.find(interfaceIndex) != node->_addrInfoClients.end());
		assert(node->_addrInfoClients[interfaceIndex] == sdRef);
#ifndef DISC_BONJOUR_EMBED
		int sockFD = DNSServiceRefSockFD(node->_addrInfoClients[interfaceIndex]);
		assert(getInstance()->_activeFDs.find(sockFD) != getInstance()->_activeFDs.end());
		getInstance()->_activeFDs.erase(sockFD);
#endif
		DNSServiceRefDeallocate(node->_addrInfoClients[interfaceIndex]);
		node->_addrInfoClients.erase(interfaceIndex);
	}
	// is this node sufficiently resolved?
	if (node->_interfacesIPv4.begin() != node->_interfacesIPv4.end()) {
		map<int, string>::const_iterator ifIter = node->_interfacesIPv4.begin();
		while(ifIter != node->_interfacesIPv4.end()) {
			if (ifIter->second.length() > 0)
				query->found(boost::static_pointer_cast<NodeStub>(node));
			ifIter++;
		}
//      node->_interfaceIndices.size() == node->_interfacesIPv6.size()) {
		// we resolved all interfaces
		//LOG_DEBUG("%p sufficiently resolved node %s", query.get(), SHORT_UUID(node->getUUID()).c_str());
	}

	assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
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
void DNSSD_API BonjourNodeDiscovery::registerReply(
    DNSServiceRef            sdRef,
    DNSServiceFlags          flags,
    DNSServiceErrorType      errorCode,
    const char               *name,
    const char               *regtype,
    const char               *domain,
    void                     *context // address of node
) {

	LOG_DEBUG("registerReply: %s %s/%s as %s", (flags & kDNSServiceFlagsAdd ? "Registered" : "Unregistered"), strndup(name, 8), domain, regtype);
	if (errorCode == kDNSServiceErr_NameConflict)
		LOG_WARN("Name conflict with UUIDs?!");

	switch (errorCode) {
	case kDNSServiceErr_NoError: {
		// update local node with information from registration
		shared_ptr<NodeImpl> node = getInstance()->_localNodes[(intptr_t)context];
		assert(node != NULL);
		assert(name != NULL);
		assert(domain != NULL);
		assert(node->getUUID().compare(name) == 0);
		node->setDomain(domain);
	}
	break;
	case kDNSServiceErr_NameConflict:
		break;
	default:
		break;
	}
	assert(getInstance()->validateState());
	return;
}

/**
 * Whenever one of our member functions is left, all the assumptions in this function have to hold.
 */
bool BonjourNodeDiscovery::validateState() {

	UMUNDO_LOCK(_mutex);
	map<int, DNSServiceRef>::iterator activeFDIter;
	map<intptr_t, shared_ptr<NodeImpl> >::iterator localNodeIter;
	map<intptr_t, DNSServiceRef>::iterator registerClientIter;
	map<intptr_t, shared_ptr<NodeQuery> >::iterator queryIter;
	map<intptr_t, DNSServiceRef>::iterator queryClientIter;
	map<intptr_t, shared_ptr<NodeQuery> >::iterator nodeToQueryIter;
	map<shared_ptr<NodeQuery>, map<string, shared_ptr<BonjourNodeStub> > >::iterator queryToNodeIter;

	map<string, DNSServiceRef>::iterator serviceResolveClientIter;
	map<int, DNSServiceRef>::iterator addrInfoClientIter;

	// make sure socket file descriptors are consistent - noop in embedded
#ifndef DISC_BONJOUR_EMBED
	set<int> socketFDs;
	for (activeFDIter = _activeFDs.begin(); activeFDIter != _activeFDs.end(); activeFDIter++) {
		// key is file descriptor of bonjour handle
		assert(activeFDIter->first == DNSServiceRefSockFD(activeFDIter->second));
		// gather all socket fds to eliminate them in subsequent tests
		socketFDs.insert(activeFDIter->first);
	}
#endif

	for (localNodeIter = _localNodes.begin(); localNodeIter != _localNodes.end(); localNodeIter++) {
		// assume that we have a dns register client
		assert(_registerClients.find(localNodeIter->first) != _registerClients.end());
	}
	assert(_registerClients.size() == _localNodes.size());

	// test register clients
	for (registerClientIter = _registerClients.begin(); registerClientIter != _registerClients.end(); registerClientIter++) {
#ifndef DISC_BONJOUR_EMBED
		assert(socketFDs.find(DNSServiceRefSockFD(registerClientIter->second)) != socketFDs.end()); // client is in active set
		socketFDs.erase(DNSServiceRefSockFD(registerClientIter->second));
#endif
		// there is a node to the register query
		assert(_localNodes.find(registerClientIter->first) != _localNodes.end());
	}

	for (queryIter = _queries.begin(); queryIter != _queries.end(); queryIter++) {
		// we have a bonjour query running for this query
		assert(_queryClients.find(queryIter->first) != _queryClients.end());
		// we have this queries socket in all sockets
#ifndef DISC_BONJOUR_EMBED
		assert(socketFDs.find(DNSServiceRefSockFD(_queryClients[queryIter->first])) != socketFDs.end());
		socketFDs.erase(DNSServiceRefSockFD(_queryClients[queryIter->first]));
#endif
	}
	// there are as many queries as there are query clients in bonjour
	assert(_queryClients.size() == _queries.size());


	for (queryToNodeIter = _queryToNodes.begin(); queryToNodeIter != _queryToNodes.end(); queryToNodeIter++) {
		shared_ptr<NodeQuery> query = queryToNodeIter->first;
		map<string, shared_ptr<BonjourNodeStub> > remoteNodes = queryToNodeIter->second;
		assert(_queries.find((intptr_t)query.get()) != _queries.end());
		map<string, shared_ptr<BonjourNodeStub> >::iterator remoteNodeIter;
		for (remoteNodeIter = remoteNodes.begin(); remoteNodeIter != remoteNodes.end(); remoteNodeIter++) {
			shared_ptr<BonjourNodeStub> remoteNode = remoteNodeIter->second;
			assert(remoteNode->getDomain().find(query->getDomain()) >= 0);
			map<string, DNSServiceRef>::iterator serviceResolverIter;
			for (serviceResolverIter = remoteNode->_serviceResolveClients.begin(); serviceResolverIter != remoteNode->_serviceResolveClients.end(); serviceResolverIter++) {
				// every service resolver is in the active FDs
#ifndef DISC_BONJOUR_EMBED
				assert(socketFDs.find(DNSServiceRefSockFD(serviceResolverIter->second)) != socketFDs.end());
				socketFDs.erase(DNSServiceRefSockFD(serviceResolverIter->second));
#endif
			}

			map<int, DNSServiceRef>::iterator adressResolverIter;
			for (adressResolverIter = remoteNodeIter->second->_addrInfoClients.begin(); adressResolverIter != remoteNodeIter->second->_addrInfoClients.end(); adressResolverIter++) {
				// every service resolver is in the active FDs
#ifndef DISC_BONJOUR_EMBED
				assert(socketFDs.find(DNSServiceRefSockFD(adressResolverIter->second)) != socketFDs.end());
				socketFDs.erase(DNSServiceRefSockFD(adressResolverIter->second));
#endif
			}
		}
	}

#ifndef DISC_BONJOUR_EMBED
	if (socketFDs.size() > 0) {
		set<int>::iterator socketFDIter;
		for (socketFDIter = socketFDs.begin(); socketFDIter != socketFDs.end(); socketFDIter++) {
			LOG_ERR("Pending filehandle %d", (*socketFDIter));
		}
	}
	assert(socketFDs.size() == 0);
#endif

//  LOG_DEBUG("Validated state: %d remote nodes, %d local nodes, %d queries", _nodeToQuery.size(), _localNodes.size(), _activeFDs.size());
	UMUNDO_UNLOCK(_mutex);
	return true;
}

}
