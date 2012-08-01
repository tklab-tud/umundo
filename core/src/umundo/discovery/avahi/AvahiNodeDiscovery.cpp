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

#include "umundo/discovery/avahi/AvahiNodeDiscovery.h"

#include <stdio.h> // asprintf
#include <string.h> // strndup

#include "umundo/connection/Node.h"
#include "umundo/discovery/NodeQuery.h"
#include "umundo/discovery/avahi/AvahiNodeStub.h"
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>

namespace umundo {

shared_ptr<Implementation> AvahiNodeDiscovery::create(void*) {
	return getInstance();
}

shared_ptr<AvahiNodeDiscovery> AvahiNodeDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		_instance = shared_ptr<AvahiNodeDiscovery>(new AvahiNodeDiscovery());
		(_instance->_simplePoll = avahi_simple_poll_new()) || LOG_WARN("avahi_simple_poll_new");
	}
	return _instance;
}
shared_ptr<AvahiNodeDiscovery> AvahiNodeDiscovery::_instance;

AvahiNodeDiscovery::AvahiNodeDiscovery() {
}

void AvahiNodeDiscovery::destroy() {
}

void AvahiNodeDiscovery::init(shared_ptr<Configuration>) {
}

/**
 * Unregister all nodes when suspending.
 *
 * When we are suspended, we unadvertise all registered nodes and save them as
 * _suspendedNodes to add them when resuming.
 */
void AvahiNodeDiscovery::suspend() {
	UMUNDO_LOCK(_mutex);
	if (_isSuspended) {
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	_isSuspended = true;

	// remove all nodes from discovery
	_suspendedNodes = _nodes;
	map<intptr_t, shared_ptr<NodeImpl> >::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
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
void AvahiNodeDiscovery::resume() {
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

AvahiNodeDiscovery::~AvahiNodeDiscovery() {
}

void AvahiNodeDiscovery::remove(shared_ptr<NodeImpl> node) {
	UMUNDO_LOCK(_mutex);
	LOG_INFO("Removing node %s", SHORT_UUID(node->getUUID()).c_str());
	intptr_t address = (intptr_t)(node.get());
	if(_avahiClients.find(address) == _avahiClients.end()) {
		LOG_WARN("Ignoring removal of unregistered node from discovery");
		UMUNDO_UNLOCK(_mutex);
		return;
	}
	assert(_avahiClients.find(address) != _avahiClients.end());
	avahi_client_free(_avahiClients[address]);
	_avahiClients.erase(address);
	assert(_avahiGroups.find(address) != _avahiGroups.end());
//	avahi_entry_group_free(_avahiGroups[address]);
	_avahiGroups.erase(address);
	assert(_nodes.find(address) != _nodes.end());
	_nodes.erase(address);
	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}

void AvahiNodeDiscovery::add(shared_ptr<NodeImpl> node) {
	UMUNDO_LOCK(_mutex);
	LOG_INFO("Adding node %s", SHORT_UUID(node->getUUID()).c_str());
	int err;
	intptr_t address = (intptr_t)(node.get());

	if (_nodes.find(address) != _nodes.end()) {
		// there has to be a register client if we know the node
		assert(_avahiClients.find(address) != _avahiClients.end());
		LOG_WARN("Ignoring addition of node already added to discovery");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}

	_nodes[address] = node;

	AvahiClient* client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, &clientCallback, (void*)address, &err);
	if (!client) LOG_WARN("avahi_client_new - is the Avahi daemon running?", err);
	assert(_avahiClients.find(address) == _avahiClients.end());
	_avahiClients[address] = client;
	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
	start();
}

void AvahiNodeDiscovery::unbrowse(shared_ptr<NodeQuery> query) {
	UMUNDO_LOCK(_mutex);
	intptr_t address = (intptr_t)(query.get());

	LOG_INFO("Removing query %p for nodes in '%s'", address, query->getDomain().c_str());

	if(_avahiClients.find(address) == _avahiClients.end()) {
		LOG_WARN("Unbrowsing query that was never added");
		UMUNDO_UNLOCK(_mutex);
		return;
	}

	assert(_avahiBrowsers.find(address) != _avahiBrowsers.end());
	assert(_avahiClients.find(address) != _avahiClients.end());
	avahi_service_browser_free(_avahiBrowsers[address]);
	avahi_client_free(_avahiClients[address]);
	_avahiBrowsers.erase(address);
	_avahiClients.erase(address);
	_queryNodes.erase(query);
	_browsers.erase(address);

	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}

void AvahiNodeDiscovery::browse(shared_ptr<NodeQuery> query) {
	UMUNDO_LOCK(_mutex);
	AvahiServiceBrowser *sb = NULL;
	AvahiClient *client = NULL;
	intptr_t address = (intptr_t)(query.get());

	if (_browsers.find(address) != _browsers.end()) {
		LOG_WARN("Already browsing for given query");
		UMUNDO_UNLOCK(_mutex);
		assert(validateState());
		return;
	}

	int error;

	LOG_INFO("Adding query %p for nodes in '%s'", address, query->getDomain().c_str());

	client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, browseClientCallback, (void*)address, &error);
	if (client == NULL) {
		LOG_ERR("avahi_client_new failed - is the Avahi daemon running?", error);
		assert(validateState());
		UMUNDO_UNLOCK(_mutex);
		return;
	}

	char* domain;
	if (strstr(query->getDomain().c_str(), ".") != NULL) {
		asprintf(&domain, "%s", query->getDomain().c_str());
	} else if (query->getDomain().length() > 0) {
		asprintf(&domain, "%s.local.", query->getDomain().c_str());
	} else {
		asprintf(&domain, "local.");
	}

	char* regtype;
	const char* transport = (query->getTransport().length() ? query->getTransport().c_str() : "tcp");
	asprintf(&regtype, "_mundo._%s", transport);

	if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, regtype, domain, (AvahiLookupFlags)0, browseCallback, (void*)address))) {
		LOG_WARN("avahi_service_browser_new failed %s", avahi_strerror(error));
		assert(validateState());
		UMUNDO_UNLOCK(_mutex);
		return;
	}

	assert(_avahiClients.find(address) == _avahiClients.end());
	assert(_browsers.find(address) == _browsers.end());
	assert(_avahiBrowsers.find(address) == _avahiBrowsers.end());
	_avahiClients[address] = client;
	_browsers[address] = query;
	_avahiBrowsers[address] = sb;

	free(domain);
	free(regtype);
	start();
	assert(validateState());
	UMUNDO_UNLOCK(_mutex);
}

/**
 * Called whenever we register a new query with Avahi.
 */
void AvahiNodeDiscovery::browseClientCallback(AvahiClient* c, AvahiClientState state, void* queryAddr) {
	UMUNDO_LOCK(getInstance()->_mutex);
	assert(c);
	// there is not yet a query there
//	assert(getInstance()->_browsers.find((intptr_t)queryAddr) != getInstance()->_browsers.end());

	switch(state) {
	case AVAHI_CLIENT_CONNECTING:
		LOG_WARN("browseClientCallback: Query %p - client still connecting %s", queryAddr, avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_FAILURE:
		LOG_WARN("browseClientCallback: Query %p - server connection failure %s", queryAddr, avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_RUNNING:
		LOG_INFO("browseClientCallback: Query %p - server state: RUNNING %s", queryAddr, avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_REGISTERING:
		LOG_WARN("browseClientCallback: Query %p - server state: REGISTERING %s", queryAddr, avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_COLLISION:
		LOG_WARN("browseClientCallback: Query %p - server state: COLLISION %s", queryAddr, avahi_strerror(avahi_client_errno(c)));
		break;
	default:
		LOG_WARN("Unknown state");
		break;
	}
//	assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
}

/**
 * Called whenever something is happening for one of our registered queries.
 *
 * Triggers resolving new nodes with their reported interfaces and protocol.
 */
void AvahiNodeDiscovery::browseCallback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AvahiLookupResultFlags flags,
    void* userdata
) {
	UMUNDO_LOCK(getInstance()->_mutex);
	LOG_DEBUG("browseCallback: %s %s/%s as %s at if %d with protocol %d",
	          (event == AVAHI_BROWSER_NEW ? "Added" : "Called for"),
	          (name == NULL ? "NULL" : strndup(name, 8)),
	          (domain == NULL ? "NULL" : domain),
	          type,
	          interface,
	          protocol);

	shared_ptr<AvahiNodeDiscovery> myself = getInstance();
	assert(myself->_browsers.find((intptr_t)userdata) != myself->_browsers.end());
	assert(myself->_avahiClients.find((intptr_t)userdata) != myself->_avahiClients.end());
	shared_ptr<NodeQuery> query = myself->_browsers[(intptr_t)userdata];
	AvahiClient* client = myself->_avahiClients[(intptr_t)userdata];

	switch (event) {
	case AVAHI_BROWSER_NEW: {
		// someone is announcing a new node
		assert(name != NULL);
		shared_ptr<AvahiNodeStub> node;
		bool knownNode = false;
		if (myself->_queryNodes[query].find(name) != myself->_queryNodes[query].end()) {
			node = myself->_queryNodes[query][name];
			assert(node.get() != NULL);
			knownNode = true;
		}

		if (!knownNode) {
			// we found ourselves a new node
			node = shared_ptr<AvahiNodeStub>(new AvahiNodeStub());
			if (strncmp(type + strlen(type) - 3, "tcp", 3) == 0) {
				node->setTransport("tcp");
			} else if (strncmp(type + strlen(type) - 3, "udp", 3) == 0) {
				node->setTransport("udp");
			} else {
				LOG_WARN("Unknown transport %s, defaulting to tcp", type + strlen(type) - 3);
				node->setTransport("tcp");
			}
			node->_uuid = name;
			node->_domain = domain;
			node->_interfaceIndices.insert(interface);
			myself->_queryNodes[query][name] = node;
		}
		if (!(avahi_service_resolver_new(client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, resolveCallback, userdata)))
			LOG_WARN("avahi_service_resolver_new failed for query %p: %s",userdata, avahi_strerror(avahi_client_errno(client)));
		break;
	}
	case AVAHI_BROWSER_REMOVE: {
		assert(name != NULL);
		if (myself->_queryNodes[query].find(name) == myself->_queryNodes[query].end()) {
			LOG_INFO("Node '%s' already removed of type '%s' in domain '%s' at iface %d with proto %d", strndup(name, 8), type, domain, interface, protocol);
			assert(getInstance()->validateState());
			UMUNDO_UNLOCK(getInstance()->_mutex);
			return;
		}

		shared_ptr<AvahiNodeStub> node = myself->_queryNodes[query][name];
		LOG_INFO("Removing node '%s' of type '%s' in domain '%s' at iface %d with proto %d", strndup(name, 8), type, domain, interface, protocol);
		if (protocol == AVAHI_PROTO_INET6)
			node->_interfacesIPv6.erase(interface);
		if (protocol == AVAHI_PROTO_INET)
			node->_interfacesIPv4.erase(interface);
		if (node->_interfacesIPv4.find(interface) == node->_interfacesIPv4.end() && node->_interfacesIPv6.find(interface) == node->_interfacesIPv6.end())
			node->_interfaceIndices.erase(interface);
		if (node->_interfaceIndices.empty()) {
			LOG_INFO("Query %p vanished node %s", userdata, SHORT_UUID(node->getUUID()).c_str());
			query->removed(node);
			myself->_queryNodes[query].erase(name);
		}
		// TODO: remove from query nodes
		break;
	}
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		break;
	case AVAHI_BROWSER_ALL_FOR_NOW: {
		// TODO: This is a better place to notify the query listener
		break;
	}
	case AVAHI_BROWSER_FAILURE:
		LOG_WARN("avahi browser failure: %s", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	}
	assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
}

void AvahiNodeDiscovery::resolveCallback(
    AvahiServiceResolver *r,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    void* queryAddr
) {
	UMUNDO_LOCK(getInstance()->_mutex);
	LOG_DEBUG("resolveCallback: %s %s/%s:%d as %s at if %d with protocol %d",
	          (host_name == NULL ? "NULL" : host_name),
	          (name == NULL ? "NULL" : strndup(name, 8)),
	          (domain == NULL ? "NULL" : domain),
	          port,
	          type,
	          interface,
	          protocol);

	shared_ptr<AvahiNodeDiscovery> myself = getInstance();
	assert(myself->_browsers.find((intptr_t)queryAddr) != myself->_browsers.end());
	assert(myself->_avahiClients.find((intptr_t)queryAddr) != myself->_avahiClients.end());
	shared_ptr<NodeQuery> query = myself->_browsers[(intptr_t)queryAddr];

	if (myself->_queryNodes[query].find(name) == myself->_queryNodes[query].end()) {
		LOG_WARN("resolveCallback: %p resolved unknown node %s", (name == NULL ? "NULL" : strndup(name, 8)));
		UMUNDO_UNLOCK(getInstance()->_mutex);
		return;
	}

	assert(myself->_queryNodes[query].find(name) != myself->_queryNodes[query].end());
	AvahiClient* client = myself->_avahiClients[(intptr_t)queryAddr];
	shared_ptr<AvahiNodeStub> node = myself->_queryNodes[query][name];

	(query.get() != NULL) || LOG_ERR("no browser known for queryAddr %p", queryAddr);
	(client != NULL) || LOG_ERR("no client known for queryAddr %p", queryAddr);
	(node.get() != NULL) || LOG_ERR("no node named %s known for query %p", strndup(name, 8), queryAddr);

	// do no ignore ipv6 anymore
	if (protocol == AVAHI_PROTO_INET6 && false) {
		LOG_INFO("Ignoring %s IPv6", host_name);
		node->_interfacesIPv6[interface] = "-1";
		assert(getInstance()->validateState());
		UMUNDO_UNLOCK(getInstance()->_mutex);
		return;
	}

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		LOG_WARN("resolving %s at if %d for proto %s failed: %s",
		         strndup(name, 8),
		         interface,
		         (protocol == AVAHI_PROTO_INET6 ? "IPv6" : "IPv4"),
		         avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
		break;

	case AVAHI_RESOLVER_FOUND: {
		char addr[AVAHI_ADDRESS_STR_MAX], *t;
		t = avahi_string_list_to_string(txt);
		avahi_address_snprint(addr, sizeof(addr), address);
		if (protocol == AVAHI_PROTO_INET) {
			LOG_DEBUG("resolveCallback ipv4 for iface %d: %s", interface, addr);
			// Sometimes avahi reports an ipv6 address as a ipv4 address
			char* start = addr;
			int dots = 0;
			while ((start = strchr(++start, '.')) != NULL) {
				dots++;
			}
			if (dots != 3) {
				LOG_ERR("Avahi bug: %s is not an ipv4 address!", addr);
			} else {
				node->_interfacesIPv4[interface] = addr;
			}
		} else if (protocol == AVAHI_PROTO_INET6) {
			LOG_DEBUG("resolveCallback ipv6 for iface %d: %s", interface, addr);
			node->_interfacesIPv6[interface] = addr;
		} else {
			LOG_WARN("protocol is neither ipv4 nor ipv6");
		}
		node->_isRemote = !(flags & AVAHI_LOOKUP_RESULT_LOCAL);
		node->_isOurOwn = !(flags & AVAHI_LOOKUP_RESULT_OUR_OWN);
		node->_isWan = !(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA);
		node->_txtRecord = t;
		node->_host = host_name;
		node->_port = port;
		avahi_free(t);

		if (node->_interfacesIPv4.begin() != node->_interfacesIPv4.end()) {
			query->found(node);
		}
		break;
	}
	default:
		LOG_WARN("Unknown event in resolveCallback", 0);
	}

	avahi_service_resolver_free(r);
	assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
}

/**
 * Called from Avahi whenever we add a node to discovery.
 */
void AvahiNodeDiscovery::entryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, void* userdata) {
	UMUNDO_LOCK(getInstance()->_mutex);

	shared_ptr<AvahiNodeDiscovery> myself = getInstance();
	assert(myself->_nodes.find((intptr_t)userdata) != myself->_nodes.end());

	AvahiEntryGroup* group = NULL;
	if (myself->_avahiGroups.find((intptr_t)userdata) != myself->_avahiGroups.end())
		group = myself->_avahiGroups[(intptr_t)userdata];

	assert(g == group || group == NULL);
	myself->_avahiGroups[(intptr_t)userdata] = g;

	/* Called whenever the entry group state changes */
	switch (state) {
	case AVAHI_ENTRY_GROUP_ESTABLISHED :
		LOG_DEBUG("entryGroupCallback: state AVAHI_ENTRY_GROUP_ESTABLISHED");
		/* The entry group has been established successfully */
		//std::cout << "Service successfully established: " << node->getUUID() << std::endl;
		break;

	case AVAHI_ENTRY_GROUP_COLLISION :
		LOG_WARN("entryGroupCallback state AVAHI_ENTRY_GROUP_COLLISION: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;

	case AVAHI_ENTRY_GROUP_FAILURE :
		LOG_WARN("entryGroupCallback state AVAHI_ENTRY_GROUP_FAILURE: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		/* Some kind of failure happened while we were registering our services */
		break;

	case AVAHI_ENTRY_GROUP_UNCOMMITED:
		LOG_INFO("entryGroupCallback state AVAHI_ENTRY_GROUP_UNCOMMITED: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;

	case AVAHI_ENTRY_GROUP_REGISTERING:
		LOG_INFO("entryGroupCallback state AVAHI_ENTRY_GROUP_REGISTERING: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;
	default:
		LOG_WARN("entryGroupCallback default switch should not happen!", 0);
	}
	// we are being called from avahi_client_new in add(Node), state is not yet valid
	UMUNDO_UNLOCK(getInstance()->_mutex);
}


void AvahiNodeDiscovery::clientCallback(AvahiClient* c, AvahiClientState state, void* nodeAddr) {
	UMUNDO_LOCK(getInstance()->_mutex);

	assert(c);
	int err;
	shared_ptr<AvahiNodeDiscovery> myself = getInstance();

	shared_ptr<NodeImpl> node;
	if (myself->_nodes.find((intptr_t)nodeAddr) != myself->_nodes.end())
		node = myself->_nodes[(intptr_t)nodeAddr];

	assert(node.get() != NULL);
	AvahiEntryGroup* group = NULL;
	if (myself->_avahiGroups.find((intptr_t)nodeAddr) != myself->_avahiGroups.end()) {
		group = myself->_avahiGroups[(intptr_t)nodeAddr];
	}

	switch (state) {
	case AVAHI_CLIENT_S_RUNNING:
		LOG_DEBUG("clientCallback: state AVAHI_CLIENT_S_RUNNING %s", avahi_strerror(avahi_client_errno(c)));
		/* The server has startup successfully and registered its host
		 * name on the network, so it's time to create our services */
		if (!group) {
			LOG_DEBUG("clientCallback: establishing new group");
			if (!(group = avahi_entry_group_new(c, entryGroupCallback, nodeAddr))) {
				LOG_WARN("avahi_entry_group_new failed: %s", avahi_strerror(avahi_client_errno(c)));
			} else {
				myself->_avahiGroups[(intptr_t)nodeAddr] = group;
			}
		}
		if (avahi_entry_group_is_empty(group)) {
			LOG_DEBUG("clientCallback: avahi_entry_group_is_empty");

			char* domain;
			if (strstr(node->getDomain().c_str(), ".") != NULL) {
				asprintf(&domain, "%s", node->getDomain().c_str());
			} else if (node->getDomain().length() > 0) {
				asprintf(&domain, "%s.local.", node->getDomain().c_str());
			} else {
				asprintf(&domain, "local.");
			}

			char* regtype;
			const char* transport = (node->getTransport().length() ? node->getTransport().c_str() : "tcp");
			asprintf(&regtype, "_mundo._%s", transport);

			LOG_DEBUG("clientCallback: avahi_entry_group_add_service in domain %s", domain);
			if ((err = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, node->getUUID().c_str(), regtype, domain, NULL, node->getPort(), NULL)) < 0) {
				LOG_WARN("clientCallback: avahi_entry_group_add_service failed: %s", avahi_strerror(err));
			}
			free(domain);
			free(regtype);
			/* Tell the server to register the service */
			if ((err = avahi_entry_group_commit(group)) < 0) {
				LOG_WARN("clientCallback: avahi_entry_group_commit failed: %s", avahi_strerror(err));
			}
		}
		break;

	case AVAHI_CLIENT_FAILURE:
		LOG_WARN("clientCallback AVAHI_CLIENT_FAILURE: %s", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(myself->_simplePoll);
		break;

	case AVAHI_CLIENT_S_COLLISION:
		LOG_WARN("clientCallback AVAHI_CLIENT_S_COLLISION: %s", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_REGISTERING:
		/* The server records are now being established. This
		 * might be caused by a host name change. We need to wait
		 * for our own records to register until the host name is
		 * properly esatblished. */
		LOG_INFO("clientCallback AVAHI_CLIENT_S_REGISTERING: %s", avahi_strerror(avahi_client_errno(c)));
		if (group)
			avahi_entry_group_reset(group);
		break;

	case AVAHI_CLIENT_CONNECTING:
		LOG_INFO("clientCallback AVAHI_CLIENT_CONNECTING: %s", avahi_strerror(avahi_client_errno(c)));
		break;
	}
	// we are being called from avahi_client_new in add(Node), state is not valid as such
	//assert(getInstance()->validateState());
	UMUNDO_UNLOCK(getInstance()->_mutex);
}

void AvahiNodeDiscovery::run() {
	while (isStarted()) {
		UMUNDO_LOCK(_mutex);
		int timeoutMs = 50;
		avahi_simple_poll_iterate(_simplePoll, timeoutMs) && LOG_WARN("avahi_simple_poll_iterate: %d", errno);
		UMUNDO_UNLOCK(_mutex);
		Thread::yield();
	}
}

bool AvahiNodeDiscovery::validateState() {
#if 0
	map<intptr_t, shared_ptr<NodeQuery> > _browsers;       ///< memory addresses of queries for static callbacks
	map<intptr_t, shared_ptr<NodeImpl> > _nodes;	         ///< memory addresses of local nodes for static callbacks
	map<intptr_t, shared_ptr<NodeImpl> > _suspendedNodes;	 ///< memory addresses of suspended local nodes
	map<intptr_t, AvahiClient* > _avahiClients;            ///< memory addresses of local nodes to avahi clients
	map<intptr_t, AvahiEntryGroup* > _avahiGroups;         ///< memory addresses of local nodes to avahi groups
	map<intptr_t, AvahiServiceBrowser* > _avahiBrowsers;   ///< memory addresses of local nodes to avahi service browsers
	map<shared_ptr<NodeQuery>, map<string, shared_ptr<AvahiNodeStub> > > _queryNodes;
#endif

	// every nodequery has an avahi client and a browser
	map<intptr_t, shared_ptr<NodeQuery> >::iterator browserIter = _browsers.begin();
	while(browserIter != _browsers.end()) {
		assert(_avahiClients.find(browserIter->first) != _avahiClients.end());
		assert(_avahiBrowsers.find(browserIter->first) != _avahiBrowsers.end());
		browserIter++;
	}
	// every browser belongs to a query
	assert(_avahiBrowsers.size() == _browsers.size());

	// every node has an avahi client
	map<intptr_t, shared_ptr<NodeImpl> >::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		assert(_avahiClients.find(nodeIter->first) != _avahiClients.end());
		nodeIter++;
	}

	// there are no avahi clients but for nodes and queries
	assert(_avahiClients.size() == _browsers.size() + _nodes.size());

	// every group belongs to a node
	map<intptr_t, AvahiEntryGroup*>::iterator groupIter = _avahiGroups.begin();
	while(groupIter != _avahiGroups.end()) {
		assert(_nodes.find(groupIter->first) != _nodes.end());
		groupIter++;
	}

	return true;

	// there are only as many groups as there are nodes
	assert(_avahiGroups.size() <= _nodes.size());

	return true;
}

}
