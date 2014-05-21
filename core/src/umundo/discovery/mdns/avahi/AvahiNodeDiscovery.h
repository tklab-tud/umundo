/**
 *  @file
 *  @brief      Discovery implementation with Avahi.
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

#ifndef AVAHINODEDISCOVERY_H_GCM9GM15
#define AVAHINODEDISCOVERY_H_GCM9GM15

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>

#include "umundo/common/Common.h"
#include "umundo/thread/Thread.h"
#include "umundo/discovery/Discovery.h"

namespace umundo {

class AvahiNodeStub;

/**
 * Concrete discovery implementor for avahi (bridge pattern).
 */
class UMUNDO_API AvahiNodeDiscovery : public DiscoveryImpl, public Thread {
public:
	virtual ~AvahiNodeDiscovery();
	static shared_ptr<AvahiNodeDiscovery> getInstance();

	shared_ptr<Implementation> create();
	void init(shared_ptr<Options>);
	void suspend();
	void resume();

	void add(NodeImpl* node);
	void remove(NodeImpl* node);

	void browse(shared_ptr<NodeQuery> discovery);
	void unbrowse(shared_ptr<NodeQuery> discovery);

	void run();

private:
	AvahiNodeDiscovery();

	AvahiSimplePoll *_simplePoll;
	RMutex _mutex;
	Monitor _monitor;

	bool validateState();

	static void entryGroupCallback(AvahiEntryGroup*, AvahiEntryGroupState, void*);
	static void clientCallback(AvahiClient*, AvahiClientState, void*);
	static void browseClientCallback(AvahiClient*, AvahiClientState, void*);

	static void browseCallback(
	    AvahiServiceBrowser *b,
	    AvahiIfIndex interface,
	    AvahiProtocol protocol,
	    AvahiBrowserEvent event,
	    const char *name,
	    const char *type,
	    const char *domain,
	    AvahiLookupResultFlags flags,
	    void* userdata
	);

	static void resolveCallback(
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
	    void* userdata
	);

	map<intptr_t, shared_ptr<NodeQuery> > _browsers;       ///< memory addresses of queries for static callbacks
	map<intptr_t, NodeImpl* > _nodes;	         ///< memory addresses of local nodes for static callbacks
	map<intptr_t, NodeImpl* > _suspendedNodes;	 ///< memory addresses of suspended local nodes
	map<intptr_t, AvahiClient* > _avahiClients;            ///< memory addresses of local nodes to avahi clients
	map<intptr_t, AvahiEntryGroup* > _avahiGroups;         ///< memory addresses of local nodes to avahi groups
	map<intptr_t, AvahiServiceBrowser* > _avahiBrowsers;   ///< memory addresses of local nodes to avahi service browsers

	map<shared_ptr<NodeQuery>, map<string, shared_ptr<AvahiNodeStub> > > _queryNodes;

	uint64_t lastOperation;
	void delayOperation();

	static shared_ptr<AvahiNodeDiscovery> _instance;

	friend class AvahiNodeStub;
	friend class Factory;
};

}

#endif /* end of include guard: AVAHINODEDISCOVERY_H_GCM9GM15 */
