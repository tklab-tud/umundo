/**
 *  @file
 *  @brief      Manage Services
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

#ifndef SERVICEMANAGER_H_D2RP19OS
#define SERVICEMANAGER_H_D2RP19OS

#include <umundo/core.h>
#include <umundo/s11n.h>
#include "umundo/rpc/Service.h"

namespace umundo {

/**
 * Manage Services and connects to other ServiceManagers.
 */
class UMUNDO_API ServiceManager : public Receiver, public Connectable, public Greeter {
public:
	ServiceManager();
	virtual ~ServiceManager();

	void addService(Service*);
	void addService(Service*, ServiceDescription&);
	void removeService(Service*);

	// Connectable interface
	std::map<std::string, Publisher> getPublishers();
	std::map<std::string, Subscriber> getSubscribers();
	void addedToNode(Node& node);
	void removedFromNode(Node& node);
	std::set<Node> getNodes() {
		return _nodes;
	}

	// Greeter Interface
	void welcome(Publisher&, const SubscriberStub&);
	void farewell(Publisher&, const SubscriberStub&);

	void receive(Message* msg);

	ServiceDescription find(const ServiceFilter&);
	ServiceDescription find(const ServiceFilter&, int timeout);
	std::set<ServiceDescription> findLocal(const ServiceFilter&);
	void startQuery(const ServiceFilter&, ResultSet<ServiceDescription>*);
	void stopQuery(const ServiceFilter&);

	std::map<std::string, Monitor*> _findRequests;
	std::map<std::string, Message*> _findResponses;
	std::map<intptr_t, Service*> _svc; ///< Instances of local services
	std::map<intptr_t, ServiceDescription> _localSvcDesc; ///< Descriptions of local services
	std::map<ServiceFilter, ResultSet<ServiceDescription>*> _localQueries; ///< filters for local continuous service queries

	std::set<Node> _nodes; ///< all the nodes we were added to
	std::map<std::string, ServiceFilter> _remoteQueries; ///< UUID to remote continuous query
	std::map<std::string, std::map<std::string, ServiceDescription> > _remoteSvcDesc; ///< Remote mgrIds to channel names to descriptions of remote services
	Publisher* _svcPub;   ///< publish service queries
	Subscriber* _svcSub;  ///< subscribe to service queries
	RMutex _mutex;

	std::map<std::string, std::list<std::pair<uint64_t, Message*> > > _pendingMessages;
};

}

#endif /* end of include guard: SERVICEMANAGER_H_D2RP19OS */
