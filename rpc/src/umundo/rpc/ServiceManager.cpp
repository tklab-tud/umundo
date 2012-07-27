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

#include "umundo/rpc/ServiceManager.h"

namespace umundo {

ServiceManager::ServiceManager() {
	_svcPub = new Publisher("umundo.sd");
	_svcSub = new Subscriber("umundo.sd", this);
	_svcPub->setGreeter(this);
}

ServiceManager::~ServiceManager() {
	_svcPub->setGreeter(NULL);
	delete _svcPub;
	delete _svcSub;
}

std::set<umundo::Publisher*> ServiceManager::getPublishers() {
	set<Publisher*> pubs;
	pubs.insert(_svcPub);
	return pubs;
}
std::set<umundo::Subscriber*> ServiceManager::getSubscribers() {
	set<Subscriber*> subs;
	subs.insert(_svcSub);
	return subs;
}

/**
 * Another ServiceManager was added.
 *
 * Send all local continuous queries.
 */
void ServiceManager::welcome(Publisher*, const string nodeId, const string subId) {
	ScopeLock lock(&_mutex);
	LOG_INFO("Found new remote ServiceManager - sending %d queries", _localQueries.size());
	map<ServiceFilter*, ResultSet<ServiceDescription>*, filterCmp>::iterator queryIter = _localQueries.begin();
	while (queryIter != _localQueries.end()) {
		Message* queryMsg = queryIter->first->toMessage();
		queryMsg->putMeta("type", "serviceDiscStart");
		queryMsg->putMeta("subscriber", subId);

		_svcPub->send(queryMsg);
		delete queryMsg;
		queryIter++;
	}
}

/**
 * A ServiceManager was removed.
 */
void ServiceManager::farewell(Publisher*, const string nodeId, const string subId) {

}


void ServiceManager::addedToNode(Node* node) {
	ScopeLock lock(&_mutex);
	map<intptr_t, Service*>::iterator svcIter = _svc.begin();
	while(svcIter != _svc.end()) {
		node->connect(svcIter->second);
		svcIter++;
	}
	_nodes.insert(node);
}

void ServiceManager::removedFromNode(Node* node) {
	ScopeLock lock(&_mutex);
	if (_nodes.find(node) == _nodes.end())
		return;

	map<intptr_t, Service*>::iterator svcIter = _svc.begin();
	while(svcIter != _svc.end()) {
		node->disconnect(svcIter->second);
		svcIter++;
	}
	_nodes.erase(node);
}

/**
 * Start a query for a service.
 *
 * Remember service filter and send query to all connected ServiceManagers.
 */
void ServiceManager::startQuery(ServiceFilter* filter, ResultSet<ServiceDescription>* listener) {
	ScopeLock lock(&_mutex);
	_localQueries[filter] = listener;
	Message* queryMsg = filter->toMessage();
	queryMsg->putMeta("type", "serviceDiscStart");

	_svcPub->send(queryMsg);
	LOG_INFO("Sending new query to %d ServiceManagers", _svcPub->waitForSubscribers(0));
	delete queryMsg;
}

void ServiceManager::stopQuery(ServiceFilter* filter) {
	ScopeLock lock(&_mutex);
	if (_localQueries.find(filter) != _localQueries.end()) {
		Message* unqueryMsg = filter->toMessage();
		unqueryMsg->putMeta("type", "serviceDiscStop");
		_svcPub->send(unqueryMsg);
		delete unqueryMsg;

		_localQueries.erase(filter);
	}
}

ServiceDescription* ServiceManager::find(ServiceFilter* svcFilter) {
	Message* findMsg = svcFilter->toMessage();
	string reqId = UUID::getUUID();
	findMsg->putMeta("type", "serviceDisc");
	findMsg->putMeta("reqId", reqId.c_str());
	_svcPub->waitForSubscribers(1);
	Thread::sleepMs(1000);
	_findRequests[reqId] = Monitor();

	_svcPub->send(findMsg);
	delete findMsg;

	_findRequests[reqId].wait();
	ScopeLock lock(&_mutex);
	_findRequests.erase(reqId);

	if (_findResponses.find(reqId) != _findResponses.end()) {
		Message* foundMsg = _findResponses[reqId];
		assert(foundMsg != NULL);
		ServiceDescription* svcDesc = new ServiceDescription(foundMsg);
		svcDesc->_svcManager = this;
		_findResponses.erase(reqId);
		delete foundMsg;
		return svcDesc;
	}
	return NULL;
}

/**
 * Find local service matchig filter.
 */
std::set<ServiceDescription*> ServiceManager::findLocal(ServiceFilter* svcFilter) {
	std::set<ServiceDescription*> foundSvcs;
	map<intptr_t, ServiceDescription*>::iterator svcDescIter = _localSvcDesc.begin();
	while(svcDescIter != _localSvcDesc.end()) {
		if (svcFilter->matches(svcDescIter->second)) {
			foundSvcs.insert(svcDescIter->second);
		}
		svcDescIter++;
	}
	return foundSvcs;
}

void ServiceManager::receive(Message* msg) {
	ScopeLock lock(&_mutex);
	// is this a response for one of our requests?
	if (msg->getMeta().find("respId") != msg->getMeta().end()) {
		string respId = msg->getMeta("respId");
		if (_findRequests.find(respId) != _findRequests.end()) {
			_findResponses[respId] = new Message(*msg);
			_findRequests[respId].signal();
		}
	}

	// is someone asking for a service?
	if (msg->getMeta().find("type") != msg->getMeta().end() &&
	        msg->getMeta("type").compare("serviceDisc") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		std::set<ServiceDescription*> foundSvcs = findLocal(filter);
		delete filter;

		if (foundSvcs.size() > 0) {
			ServiceDescription* svcDesc = (*(foundSvcs.begin()));
			Message* foundMsg = svcDesc->toMessage();
			foundMsg->putMeta("respId", msg->getMeta("reqId"));
			foundMsg->putMeta("desc:channel", svcDesc->getChannelName());
			_svcPub->send(foundMsg);
			delete foundMsg;
		}
	}

	// is this the start of a continuous query?
	if (msg->getMeta().find("type") != msg->getMeta().end() &&
	        msg->getMeta("type").compare("serviceDiscStart") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		_remoteQueries[filter->_uuid] = filter;

		// do we have such a service?
		std::set<ServiceDescription*> foundSvcs = findLocal(filter);
		std::set<ServiceDescription*>::iterator svcDescIter = foundSvcs.begin();
		while(svcDescIter != foundSvcs.end()) {
			if (filter->matches(*svcDescIter)) {
				Message* foundMsg = (*svcDescIter)->toMessage();
				foundMsg->putMeta("filterId", filter->_uuid);
				foundMsg->putMeta("type", "serviceDiscFound");
				foundMsg->putMeta("desc:channel", (*svcDescIter)->getChannelName());
				_svcPub->send(foundMsg);
				delete foundMsg;
			}
			svcDescIter++;
		}
	}

	// is this the end of a continuous query?
	if (msg->getMeta().find("type") != msg->getMeta().end() &&
	        msg->getMeta("type").compare("serviceDiscStop") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		if (_remoteQueries.find(filter->_uuid) != _remoteQueries.end()) {
			delete _remoteQueries[filter->_uuid];
			_remoteQueries.erase(filter->_uuid);
		}
		delete filter;
	}

	// is this a reply to a continuous service query?
	if (msg->getMeta().find("type") != msg->getMeta().end() &&
	        (msg->getMeta("type").compare("serviceDiscFound") == 0 ||
	         msg->getMeta("type").compare("serviceDiscRemoved") == 0)) {
		// _svcQueries comparator uses filter uuid
		ServiceFilter* keyFilter = new ServiceFilter("");
		keyFilter->_uuid = msg->getMeta("filterId");
		if (_localQueries.find(keyFilter) != _localQueries.end()) {
			ResultSet<ServiceDescription>* listener = _localQueries[keyFilter];
			assert(msg->getMeta("desc:channel").size() > 0);
			if (_remoteSvcDesc.find(msg->getMeta("desc:channel")) == _remoteSvcDesc.end()) {
				_remoteSvcDesc[msg->getMeta("desc:channel")] = shared_ptr<ServiceDescription>(new ServiceDescription(msg));
				_remoteSvcDesc[msg->getMeta("desc:channel")]->_svcManager = this;
			}
			if (msg->getMeta("type").compare("serviceDiscFound") == 0) {
				listener->added(_remoteSvcDesc[msg->getMeta("desc:channel")]);
			} else {
				listener->removed(_remoteSvcDesc[msg->getMeta("desc:channel")]);
				_remoteSvcDesc.erase(msg->getMeta("desc:channel"));
			}
		}
		delete keyFilter;
	}
}

void ServiceManager::addService(Service* service) {
	addService(service, new ServiceDescription(service->getName(), map<string, string>()));
}

void ServiceManager::addService(Service* service, ServiceDescription* desc) {
	ScopeLock lock(&_mutex);

	if (service == NULL)
		return;

	intptr_t svcPtr = (intptr_t)service;
	if (_svc.find(svcPtr) != _svc.end())
		return;

	desc->_channelName = service->getChannelName();
	assert(desc->_channelName.length() > 0);

	_svc[svcPtr] = service;
	_localSvcDesc[svcPtr] = desc;

	// connect service to all nodes the manager is connected to
	std::set<Node*>::iterator nodeIter;
	nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		(*nodeIter++)->connect(service);
	}

	// iterate continuous queries and notify other service managers about matches.
	std::map<string, ServiceFilter*>::iterator filterIter = _remoteQueries.begin();
	while(filterIter != _remoteQueries.end()) {
		if (filterIter->second->matches(desc)) {
			Message* foundMsg = desc->toMessage();
			foundMsg->putMeta("filterId", filterIter->second->_uuid);
			foundMsg->putMeta("type", "serviceDiscFound");
			foundMsg->putMeta("desc:channel", desc->getChannelName());
			_svcPub->send(foundMsg);
			delete foundMsg;
		}
		filterIter++;
	}
}

void ServiceManager::removeService(Service* service) {
	ScopeLock lock(&_mutex);

	if (service == NULL)
		return;

	intptr_t svcPtr = (intptr_t)service;
	if (_svc.find(svcPtr) == _svc.end())
		return;

	std::set<Node*>::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		(*nodeIter++)->disconnect(service);
	}

	assert(_localSvcDesc.find(svcPtr)!= _localSvcDesc.end());
	ServiceDescription* desc = _localSvcDesc[svcPtr];

	// iterate continuous queries and notify other service managers about removals.
	std::map<string, ServiceFilter*>::iterator filterIter = _remoteQueries.begin();
	while(filterIter != _remoteQueries.end()) {
		if (filterIter->second->matches(desc)) {
			Message* removeMsg = desc->toMessage();
			removeMsg->putMeta("filterId", filterIter->second->_uuid);
			removeMsg->putMeta("type", "serviceDiscRemoved");
			removeMsg->putMeta("desc:channel", desc->getChannelName());
			_svcPub->send(removeMsg);
			delete removeMsg;
		}
		filterIter++;
	}

	_svc.erase(svcPtr);
	_localSvcDesc.erase(svcPtr);

}

}