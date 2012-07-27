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

#ifndef SERVICEMANAGER_H_D2RP19OS
#define SERVICEMANAGER_H_D2RP19OS

#include <umundo/core.h>
#include <umundo/s11n.h>
#include "umundo/rpc/Service.h"

namespace umundo {

struct filterCmp {
	bool operator()(const ServiceFilter* a, const ServiceFilter* b) {
		return a->_uuid.compare(b->_uuid) < 0;
	}
};

class DLLEXPORT ServiceManager : public Receiver, public Connectable, public Greeter {
public:
	ServiceManager();
	virtual ~ServiceManager();

	void addService(Service*);
	void addService(Service*, ServiceDescription*);
	void removeService(Service*);

	// Connectable interface
	std::set<umundo::Publisher*> getPublishers();
	std::set<umundo::Subscriber*> getSubscribers();
	void addedToNode(Node* node);
	void removedFromNode(Node* node);
	std::set<Node*> getNodes() {
		return _nodes;
	}

	// Greeter Interface
	void welcome(Publisher*, const string nodeId, const string subId);
	void farewell(Publisher*, const string nodeId, const string subId);

	void receive(Message* msg);

	ServiceDescription* find(ServiceFilter*);
	std::set<ServiceDescription*> findLocal(ServiceFilter*);
	void startQuery(ServiceFilter*, ResultSet<ServiceDescription>*);
	void stopQuery(ServiceFilter*);

	map<string, Monitor> _findRequests;
	map<string, Message*> _findResponses;
	map<intptr_t, Service*> _svc; ///< Instances of local services
	map<intptr_t, ServiceDescription*> _localSvcDesc; ///< Descriptions of local services
	map<string, shared_ptr<ServiceDescription> > _remoteSvcDesc; ///< Descriptions of remote services
	map<ServiceFilter*, ResultSet<ServiceDescription>*, filterCmp> _localQueries; ///< filters for local continuous service queries

	std::set<Node*> _nodes;
	std::map<string, ServiceFilter*> _remoteQueries; ///< UUID to remote continuous query
	Publisher* _svcPub;   ///< publish service queries
	Subscriber* _svcSub;  ///< subscribe to service queries
	Mutex _mutex;
};

}

#endif /* end of include guard: SERVICEMANAGER_H_D2RP19OS */
