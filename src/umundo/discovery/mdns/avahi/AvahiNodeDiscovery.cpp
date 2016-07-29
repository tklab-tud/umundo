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

shared_ptr<Implementation> AvahiNodeDiscovery::create() {
	return getInstance();
}

shared_ptr<AvahiNodeDiscovery> AvahiNodeDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		_instance = shared_ptr<AvahiNodeDiscovery>(new AvahiNodeDiscovery());
		(_instance->_simplePoll = avahi_simple_poll_new()) || UM_LOG_WARN("avahi_simple_poll_new");
	}
	return _instance;
}
shared_ptr<AvahiNodeDiscovery> AvahiNodeDiscovery::_instance;

AvahiNodeDiscovery::AvahiNodeDiscovery() {
	lastOperation = 0;
	_simplePoll = NULL;
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
	ScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;

	// remove all nodes from discovery
	_suspendedNodes = _nodes;
	map<intptr_t, NodeImpl* >::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		NodeImpl* node = nodeIter->second;
		nodeIter++;
		remove(node);
	}
}

/**
 * Re-register nodes previously suspended.
 *
 * We remembered all our nodes in _suspendedNodes when this object was suspended.
 * By explicitly calling resume on the nodes we add again, we make sure they are
 * reinitialized before advertising them.
 */
void AvahiNodeDiscovery::resume() {
	ScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;

	map<intptr_t, NodeImpl* >::iterator nodeIter = _suspendedNodes.begin();
	while(nodeIter != _suspendedNodes.end()) {
		NodeImpl* node = nodeIter->second;
		nodeIter++;
		// make sure advertised nodes are initialized
		node->resume();
		add(node);
	}
	_suspendedNodes.clear();
}

AvahiNodeDiscovery::~AvahiNodeDiscovery() {
	{
		ScopeLock lock(_mutex);
		map<intptr_t, NodeImpl* >::iterator nodeIter = _nodes.begin();
		while(nodeIter != _nodes.end()) {
			NodeImpl* node = nodeIter->second;
			nodeIter++;
			remove(node);
		}
	}
	stop();
	join();
}

void AvahiNodeDiscovery::remove(NodeImpl* node) {
	ScopeLock lock(_mutex);
	delayOperation();

	UM_LOG_INFO("Removing node %s", SHORT_UUID(node->getUUID()).c_str());
	intptr_t address = (intptr_t)node;
	if(_avahiClients.find(address) == _avahiClients.end()) {
		UM_LOG_WARN("Ignoring removal of unregistered node from discovery");
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
	_monitor.wait(_mutex);
	// avahi will report removed nodes otherwise
	Thread::sleepMs(500);
	assert(validateState());
}

void AvahiNodeDiscovery::add(NodeImpl* node) {
	ScopeLock lock(_mutex);
	delayOperation();

	UM_LOG_INFO("Adding node %s", SHORT_UUID(node->getUUID()).c_str());
	int err;
	intptr_t address = (intptr_t)node;

	if (_nodes.find(address) != _nodes.end()) {
		// there has to be a register client if we know the node
		assert(_avahiClients.find(address) != _avahiClients.end());
		UM_LOG_WARN("Ignoring addition of node already added to discovery");
		assert(validateState());
		return;
	}

	_nodes[address] = node;

	AvahiClient* client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, &clientCallback, (void*)address, &err);
	if (!client) UM_LOG_WARN("avahi_client_new - is the Avahi daemon running?", err);
	assert(_avahiClients.find(address) == _avahiClients.end());
	_avahiClients[address] = client;
	assert(validateState());

	start();

#if 1
	// report matching local nodes immediately!
	map<intptr_t, shared_ptr<NodeQuery> >::iterator queryIter = _browsers.begin();
	while(queryIter != _browsers.end()) {
		if (queryIter->second->getDomain().compare(node->getDomain()) == 0) {
			shared_ptr<AvahiNodeStub> foundNode = shared_ptr<AvahiNodeStub>(new AvahiNodeStub());
			foundNode->setInProcess(true);
			foundNode->setRemote(false);
			foundNode->setTransport(node->getTransport());
			foundNode->setPort(node->getPort());
			foundNode->setUUID(node->getUUID());
			foundNode->setDomain(node->getDomain());
			foundNode->_interfacesIPv4[0] = "127.0.0.1"; // this might be a problem somewhen

			_queryNodes[queryIter->second][node->getUUID()] = foundNode;

			queryIter->second->found(NodeStub(foundNode));
		}
		queryIter++;
	}
#endif


	_monitor.wait(_mutex);
}

void AvahiNodeDiscovery::unbrowse(shared_ptr<NodeQuery> query) {
	ScopeLock lock(_mutex);
	delayOperation();

	intptr_t address = (intptr_t)(query.get());

	UM_LOG_INFO("Removing query %p for nodes in '%s'", address, query->getDomain().c_str());

	if(_avahiClients.find(address) == _avahiClients.end()) {
		UM_LOG_WARN("Unbrowsing query that was never added");
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
}

void AvahiNodeDiscovery::browse(shared_ptr<NodeQuery> query) {
	ScopeLock lock(_mutex);
	delayOperation();

	AvahiClient *client = NULL;
	intptr_t address = (intptr_t)(query.get());

	if (_browsers.find(address) != _browsers.end()) {
		UM_LOG_WARN("Already browsing for given query");
		assert(validateState());
		return;
	}

	assert(_browsers.find(address) == _browsers.end());
	_browsers[address] = query;

	UM_LOG_INFO("Adding query %p for nodes in '%s'", address, query->getDomain().c_str());

	int error;
	client = avahi_client_new(avahi_simple_poll_get(_simplePoll), (AvahiClientFlags)0, browseClientCallback, (void*)address, &error);
	if (client == NULL || error == 0) {
		UM_LOG_ERR("avahi_client_new failed - is the Avahi daemon running?", error);
		assert(validateState());
		return;
	}

	start();
	assert(validateState());
}

/**
 * Called whenever we register a new query with Avahi.
 */
void AvahiNodeDiscovery::browseClientCallback(AvahiClient* client, AvahiClientState state, void* queryAddr) {
	ScopeLock lock(getInstance()->_mutex);
	assert(client);
	// there is not yet a query there
	assert(getInstance()->_browsers.find((intptr_t)queryAddr) != getInstance()->_browsers.end());
	shared_ptr<NodeQuery> query = getInstance()->_browsers[(intptr_t)queryAddr];

	switch(state) {
	case AVAHI_CLIENT_CONNECTING:
		UM_LOG_WARN("browseClientCallback: Query %p - client still connecting %s", queryAddr, avahi_strerror(avahi_client_errno(client)));
		break;
	case AVAHI_CLIENT_FAILURE:
		UM_LOG_WARN("browseClientCallback: Query %p - server connection failure %s", queryAddr, avahi_strerror(avahi_client_errno(client)));
		break;
	case AVAHI_CLIENT_S_RUNNING: {
		UM_LOG_INFO("browseClientCallback: Query %p - server state: RUNNING %s", queryAddr, avahi_strerror(avahi_client_errno(client)));
		int error;
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

#if 1
		// report matching local nodes immediately!
		map<intptr_t, NodeImpl* >::iterator localNodeIter = getInstance()->_nodes.begin();
		while(localNodeIter != getInstance()->_nodes.end()) {
			if (localNodeIter->second->getDomain().compare(query->getDomain()) == 0) {
				shared_ptr<AvahiNodeStub> node = shared_ptr<AvahiNodeStub>(new AvahiNodeStub());
				node->setInProcess(true);
				node->setRemote(false);
				node->setTransport(localNodeIter->second->getTransport());
				node->setPort(localNodeIter->second->getPort());
				node->setUUID(localNodeIter->second->getUUID());
				node->setDomain(domain);
				node->_interfacesIPv4[0] = "127.0.0.1"; // this might be a problem somewhen

				getInstance()->_queryNodes[query][localNodeIter->second->getUUID()] = node;
				query->found(NodeStub(node));
			}

			localNodeIter++;
		}
#endif

		AvahiServiceBrowser* sb;
		if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, regtype, domain, (AvahiLookupFlags)0, browseCallback, (void*)queryAddr))) {
			UM_LOG_WARN("avahi_service_browser_new failed %s", avahi_strerror(error));
			assert(getInstance()->validateState());
			return;
		}

		assert(getInstance()->_avahiClients.find((intptr_t)queryAddr) == getInstance()->_avahiClients.end());
		assert(getInstance()->_avahiBrowsers.find((intptr_t)queryAddr) == getInstance()->_avahiBrowsers.end());
		getInstance()->_avahiClients[(intptr_t)queryAddr] = client;
		getInstance()->_avahiBrowsers[(intptr_t)queryAddr] = sb;

		free(domain);
		free(regtype);

	}
	break;
	case AVAHI_CLIENT_S_REGISTERING:
		UM_LOG_WARN("browseClientCallback: Query %p - server state: REGISTERING %s", queryAddr, avahi_strerror(avahi_client_errno(client)));
		break;
	case AVAHI_CLIENT_S_COLLISION:
		UM_LOG_WARN("browseClientCallback: Query %p - server state: COLLISION %s", queryAddr, avahi_strerror(avahi_client_errno(client)));
		break;
	default:
		UM_LOG_WARN("Unknown state");
		break;
	}
//	assert(getInstance()->validateState());
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
	ScopeLock lock(getInstance()->_mutex);
	UM_LOG_DEBUG("browseCallback: %s %s/%s as %s at if %d with protocol %d",
	             (event == AVAHI_BROWSER_NEW ? "Added" : "Called for"),
	             (name == NULL ? "NULL" : SHORT_UUID(string(name)).c_str()),
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
				UM_LOG_WARN("Unknown transport %s, defaulting to tcp", type + strlen(type) - 3);
				node->setTransport("tcp");
			}
			node->_uuid = name;
			node->_domain = domain;
			node->_interfaceIndices.insert(interface);
			myself->_queryNodes[query][name] = node;
		}
		if (!(avahi_service_resolver_new(client, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, (AvahiLookupFlags)0, resolveCallback, userdata)))
			UM_LOG_WARN("avahi_service_resolver_new failed for query %p: %s",userdata, avahi_strerror(avahi_client_errno(client)));
		break;
	}
	case AVAHI_BROWSER_REMOVE: {
		assert(name != NULL);
		if (myself->_queryNodes[query].find(name) == myself->_queryNodes[query].end()) {
			UM_LOG_INFO("Node '%s' already removed of type '%s' in domain '%s' at iface %d with proto %d", SHORT_UUID(string(name)).c_str(), type, domain, interface, protocol);
			assert(getInstance()->validateState());
			return;
		}

		shared_ptr<AvahiNodeStub> node = myself->_queryNodes[query][name];
		UM_LOG_INFO("Removing node '%s' of type '%s' in domain '%s' at iface %d with proto %d", SHORT_UUID(string(name)).c_str(), type, domain, interface, protocol);
		if (protocol == AVAHI_PROTO_INET6)
			node->_interfacesIPv6.erase(interface);
		if (protocol == AVAHI_PROTO_INET)
			node->_interfacesIPv4.erase(interface);
		if (node->_interfacesIPv4.find(interface) == node->_interfacesIPv4.end() && node->_interfacesIPv6.find(interface) == node->_interfacesIPv6.end())
			node->_interfaceIndices.erase(interface);
		if (node->_interfaceIndices.empty()) {
			UM_LOG_INFO("Query %p vanished node %s", userdata, SHORT_UUID(node->getUUID()).c_str());
			query->removed(NodeStub(node));
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
		UM_LOG_WARN("avahi browser failure: %s", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		break;
	}
	assert(getInstance()->validateState());
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
	ScopeLock lock(getInstance()->_mutex);
	UM_LOG_DEBUG("resolveCallback: %s %s/%s:%d as %s at if %d with protocol %d",
	             (host_name == NULL ? "NULL" : host_name),
	             (name == NULL ? "NULL" : SHORT_UUID(string(name)).c_str()),
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
		UM_LOG_WARN("resolveCallback: %p resolved unknown node %s", (name == NULL ? "NULL" : SHORT_UUID(string(name)).c_str()));
		return;
	}

	assert(myself->_queryNodes[query].find(name) != myself->_queryNodes[query].end());
	AvahiClient* client = myself->_avahiClients[(intptr_t)queryAddr];
	shared_ptr<AvahiNodeStub> node = myself->_queryNodes[query][name];

	(query.get() != NULL) || UM_LOG_ERR("no browser known for queryAddr %p", queryAddr);
	(client != NULL) || UM_LOG_ERR("no client known for queryAddr %p", queryAddr);
	(node.get() != NULL) || UM_LOG_ERR("no node named %s known for query %p", SHORT_UUID(string(name)).c_str(), queryAddr);

	// do no ignore ipv6 anymore
	if (protocol == AVAHI_PROTO_INET6 && false) {
		UM_LOG_INFO("Ignoring %s IPv6", host_name);
		node->_interfacesIPv6[interface] = "-1";
		assert(getInstance()->validateState());
		return;
	}

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		UM_LOG_WARN("resolving %s at if %d for proto %s failed: %s",
		            SHORT_UUID(string(name)).c_str(),
		            interface,
		            (protocol == AVAHI_PROTO_INET6 ? "IPv6" : "IPv4"),
		            avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
		break;

	case AVAHI_RESOLVER_FOUND: {
		char addr[AVAHI_ADDRESS_STR_MAX], *t;
		t = avahi_string_list_to_string(txt);
		avahi_address_snprint(addr, sizeof(addr), address);
		if (protocol == AVAHI_PROTO_INET) {
			UM_LOG_DEBUG("resolveCallback ipv4 for iface %d: %s", interface, addr);
			// Sometimes avahi reports an ipv6 address as a ipv4 address
			char* start = addr;
			int dots = 0;
			while ((start = strchr(++start, '.')) != NULL) {
				dots++;
			}
			if (dots != 3) {
				UM_LOG_ERR("Avahi bug: %s is not an ipv4 address!", addr);
			} else {
				node->_interfacesIPv4[interface] = addr;
			}
		} else if (protocol == AVAHI_PROTO_INET6) {
			UM_LOG_DEBUG("resolveCallback ipv6 for iface %d: %s", interface, addr);
			node->_interfacesIPv6[interface] = addr;
		} else {
			UM_LOG_WARN("protocol is neither ipv4 nor ipv6");
		}
		node->_isRemote = !(flags & AVAHI_LOOKUP_RESULT_LOCAL);
		node->_isOurOwn = !(flags & AVAHI_LOOKUP_RESULT_OUR_OWN);
		node->_isWan = !(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA);
		node->_txtRecord = t;
		node->_host = host_name;
		node->_port = port;
		avahi_free(t);

		if (node->_interfacesIPv4.begin() != node->_interfacesIPv4.end()) {
			query->found(NodeStub(node));
		}
		break;
	}
	default:
		UM_LOG_WARN("Unknown event in resolveCallback");
	}

	avahi_service_resolver_free(r);
	assert(getInstance()->validateState());
}

/**
 * Called from Avahi whenever we add a node to discovery.
 */
void AvahiNodeDiscovery::entryGroupCallback(AvahiEntryGroup *g, AvahiEntryGroupState state, void* userdata) {
	ScopeLock lock(getInstance()->_mutex);

	shared_ptr<AvahiNodeDiscovery> myself = getInstance();
	assert(myself->_nodes.find((intptr_t)userdata) != myself->_nodes.end());

	AvahiEntryGroup* group = NULL;
	if (myself->_avahiGroups.find((intptr_t)userdata) != myself->_avahiGroups.end())
		group = myself->_avahiGroups[(intptr_t)userdata];

	(void)group;
	assert(g == group || group == NULL);
	myself->_avahiGroups[(intptr_t)userdata] = g;

	/* Called whenever the entry group state changes */
	switch (state) {
	case AVAHI_ENTRY_GROUP_ESTABLISHED :
		UM_LOG_DEBUG("entryGroupCallback: state AVAHI_ENTRY_GROUP_ESTABLISHED");
		/* The entry group has been established successfully */
		//std::cout << "Service successfully established: " << node->getUUID() << std::endl;
		break;

	case AVAHI_ENTRY_GROUP_COLLISION :
		UM_LOG_WARN("entryGroupCallback state AVAHI_ENTRY_GROUP_COLLISION: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;

	case AVAHI_ENTRY_GROUP_FAILURE :
		UM_LOG_WARN("entryGroupCallback state AVAHI_ENTRY_GROUP_FAILURE: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		/* Some kind of failure happened while we were registering our services */
		break;

	case AVAHI_ENTRY_GROUP_UNCOMMITED:
		UM_LOG_INFO("entryGroupCallback state AVAHI_ENTRY_GROUP_UNCOMMITED: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;

	case AVAHI_ENTRY_GROUP_REGISTERING:
//		UM_LOG_INFO("entryGroupCallback state AVAHI_ENTRY_GROUP_REGISTERING: %s", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
		break;
	default:
		UM_LOG_WARN("entryGroupCallback default switch should not happen!");
	}
	// we are being called from avahi_client_new in add(Node), state is not yet valid
}


void AvahiNodeDiscovery::clientCallback(AvahiClient* c, AvahiClientState state, void* nodeAddr) {
	ScopeLock lock(getInstance()->_mutex);

	assert(c);
	int err;
	shared_ptr<AvahiNodeDiscovery> myself = getInstance();

	NodeImpl* node;
	if (myself->_nodes.find((intptr_t)nodeAddr) != myself->_nodes.end())
		node = myself->_nodes[(intptr_t)nodeAddr];

	assert(node);
	AvahiEntryGroup* group = NULL;
	if (myself->_avahiGroups.find((intptr_t)nodeAddr) != myself->_avahiGroups.end()) {
		group = myself->_avahiGroups[(intptr_t)nodeAddr];
	}

	switch (state) {
	case AVAHI_CLIENT_S_RUNNING:
		UM_LOG_DEBUG("clientCallback: state AVAHI_CLIENT_S_RUNNING %s", avahi_strerror(avahi_client_errno(c)));
		/* The server has startup successfully and registered its host
		 * name on the network, so it's time to create our services */
		if (!group) {
			UM_LOG_DEBUG("clientCallback: establishing new group");
			if (!(group = avahi_entry_group_new(c, entryGroupCallback, nodeAddr))) {
				UM_LOG_WARN("avahi_entry_group_new failed: %s", avahi_strerror(avahi_client_errno(c)));
			} else {
				myself->_avahiGroups[(intptr_t)nodeAddr] = group;
			}
		}
		if (avahi_entry_group_is_empty(group)) {
			UM_LOG_DEBUG("clientCallback: avahi_entry_group_is_empty");

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

			UM_LOG_DEBUG("clientCallback: avahi_entry_group_add_service in domain %s", domain);
			if ((err = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, node->getUUID().c_str(), regtype, domain, NULL, node->getPort(), NULL)) < 0) {
				UM_LOG_WARN("clientCallback: avahi_entry_group_add_service failed: %s", avahi_strerror(err));
			}
			free(domain);
			free(regtype);
			/* Tell the server to register the service */
			if ((err = avahi_entry_group_commit(group)) < 0) {
				UM_LOG_WARN("clientCallback: avahi_entry_group_commit failed: %s", avahi_strerror(err));
			}
		}
		break;

	case AVAHI_CLIENT_FAILURE:
		UM_LOG_WARN("clientCallback AVAHI_CLIENT_FAILURE: %s", avahi_strerror(avahi_client_errno(c)));
		avahi_simple_poll_quit(myself->_simplePoll);
		break;

	case AVAHI_CLIENT_S_COLLISION:
		UM_LOG_WARN("clientCallback AVAHI_CLIENT_S_COLLISION: %s", avahi_strerror(avahi_client_errno(c)));
		break;
	case AVAHI_CLIENT_S_REGISTERING:
		/* The server records are now being established. This
		 * might be caused by a host name change. We need to wait
		 * for our own records to register until the host name is
		 * properly esatblished. */
		UM_LOG_INFO("clientCallback AVAHI_CLIENT_S_REGISTERING: %s", avahi_strerror(avahi_client_errno(c)));
		if (group)
			avahi_entry_group_reset(group);
		break;

	case AVAHI_CLIENT_CONNECTING:
		UM_LOG_INFO("clientCallback AVAHI_CLIENT_CONNECTING: %s", avahi_strerror(avahi_client_errno(c)));
		break;
	}
	// we are being called from avahi_client_new in add(Node), state is not valid as such
	//assert(getInstance()->validateState());
}

void AvahiNodeDiscovery::run() {
	while (isStarted()) {
		{
			ScopeLock lock(_mutex);
			int timeoutMs = 50;
			avahi_simple_poll_iterate(_simplePoll, timeoutMs) && UM_LOG_WARN("avahi_simple_poll_iterate: %d", errno);
			_monitor.signal();
		}
		Thread::sleepMs(20);
//		Thread::yield();
	}
}

bool AvahiNodeDiscovery::validateState() {
#if 0
	map<intptr_t, shared_ptr<NodeQuery> > _browsers;       ///< memory addresses of queries for static callbacks
	map<intptr_t, NodeImpl* > _nodes;	         ///< memory addresses of local nodes for static callbacks
	map<intptr_t, NodeImpl* > _suspendedNodes;	 ///< memory addresses of suspended local nodes
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
	map<intptr_t, NodeImpl* >::iterator nodeIter = _nodes.begin();
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

void AvahiNodeDiscovery::delayOperation() {
	uint64_t diff;
	long minDelay = 300;
	while((diff = Thread::getTimeStampMs() - lastOperation) < minDelay) {
		Thread::sleepMs(minDelay - diff);
	}
	lastOperation = Thread::getTimeStampMs();
}

}
