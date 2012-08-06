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
	LOG_INFO("found remote ServiceManager - sending %d queries", _localQueries.size());
	map<ServiceFilter*, ResultSet<ServiceDescription>*, ServiceFilter::filterCmp>::iterator queryIter = _localQueries.begin();
	while (queryIter != _localQueries.end()) {
		Message* queryMsg = queryIter->first->toMessage();
		queryMsg->putMeta("um.rpc.type", "serviceDiscStart");
		queryMsg->putMeta("um.sub", subId);
		queryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
		_svcPub->send(queryMsg);
		delete queryMsg;
		queryIter++;
	}
}

/**
 * A ServiceManager was removed.
 */
void ServiceManager::farewell(Publisher*, const string nodeId, const string subId) {
  LOG_INFO("removed remote ServiceManager - notifying", _localQueries.size());

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
	queryMsg->putMeta("um.rpc.type", "startDiscovery");
	queryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());

	_svcPub->send(queryMsg);
	LOG_INFO("Sending new query to %d ServiceManagers", _svcPub->waitForSubscribers(0));
	delete queryMsg;
}

void ServiceManager::stopQuery(ServiceFilter* filter) {
	ScopeLock lock(&_mutex);
	if (_localQueries.find(filter) != _localQueries.end()) {
		Message* unqueryMsg = filter->toMessage();
		unqueryMsg->putMeta("um.rpc.type", "stopDiscovery");
		unqueryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
		_svcPub->send(unqueryMsg);
		delete unqueryMsg;

		_localQueries.erase(filter);
	}
}

ServiceDescription* ServiceManager::find(ServiceFilter* svcFilter) {
	if(_svcPub->waitForSubscribers(1, 3000) < 1) {
    // there is no other ServiceManager yet
    LOG_INFO("Failed to find another ServiceManager");
    return NULL;
  }

	Message* findMsg = svcFilter->toMessage();
	string reqId = UUID::getUUID();
	findMsg->putMeta("um.rpc.type", "discover");
	findMsg->putMeta("um.rpc.reqId", reqId.c_str());
	findMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
//	Thread::sleepMs(1000);
	if(_findRequests.find(reqId) != _findRequests.end())
		LOG_WARN("Find request %s already received", reqId.c_str());
	_findRequests[reqId] = new Monitor();

	_svcPub->send(findMsg);
	delete findMsg;

	_findRequests[reqId]->wait(3000);
	ScopeLock lock(&_mutex);
	delete _findRequests[reqId];
	_findRequests.erase(reqId);

	if (_findResponses.find(reqId) == _findResponses.end()) {
    LOG_INFO("Failed to find %s", svcFilter->getServiceName().c_str());
    return NULL;
  }

  // TODO: Remove other replies as they come in!

  Message* foundMsg = _findResponses[reqId];
  assert(foundMsg != NULL);
  ServiceDescription* svcDesc = new ServiceDescription(foundMsg);
  svcDesc->_svcManager = this;
  _findResponses.erase(reqId);
  delete foundMsg;
  return svcDesc;

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
			LOG_INFO("we have a match for %s", svcFilter->getServiceName().c_str());
		}
		svcDescIter++;
	}
	return foundSvcs;
}

void ServiceManager::receive(Message* msg) {
	ScopeLock lock(&_mutex);
	// is this a response for one of our requests?
	if (msg->getMeta().find("um.rpc.respId") != msg->getMeta().end()) {
		string respId = msg->getMeta("um.rpc.respId");
		if (_findRequests.find(respId) != _findRequests.end()) {
			_findResponses[respId] = new Message(*msg);
			_findRequests[respId]->signal();
		}
	}

	// is someone simply asking for a service via find?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("discover") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		std::set<ServiceDescription*> foundSvcs = findLocal(filter);
		delete filter;

		if (foundSvcs.size() > 0) {
			ServiceDescription* svcDesc = (*(foundSvcs.begin()));
			Message* foundMsg = svcDesc->toMessage();
			foundMsg->putMeta("um.rpc.respId", msg->getMeta("um.rpc.reqId"));
			foundMsg->putMeta("um.rpc.channel", svcDesc->getChannelName());
			foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
			_svcPub->send(foundMsg);
			delete foundMsg;
		}
	}

	// is this the start of a continuous query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("startDiscovery") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		_remoteQueries[filter->_uuid] = filter;

		LOG_INFO("Received query for '%s'", filter->_svcName.c_str());

		// do we have such a service?
		std::set<ServiceDescription*> foundSvcs = findLocal(filter);
		std::set<ServiceDescription*>::iterator svcDescIter = foundSvcs.begin();
		while(svcDescIter != foundSvcs.end()) {
			if (filter->matches(*svcDescIter)) {
				Message* foundMsg = (*svcDescIter)->toMessage();
				foundMsg->putMeta("um.rpc.filterId", filter->_uuid);
				foundMsg->putMeta("um.rpc.type", "discovered");
				foundMsg->putMeta("um.rpc.channel", (*svcDescIter)->getChannelName());
				foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
				_svcPub->send(foundMsg);
				delete foundMsg;
			}
			svcDescIter++;
		}
	}

	// is this the end of a continuous query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("stopDiscovery") == 0) {
		ServiceFilter* filter = new ServiceFilter(msg);
		if (_remoteQueries.find(filter->_uuid) != _remoteQueries.end()) {
			delete _remoteQueries[filter->_uuid];
			_remoteQueries.erase(filter->_uuid);
		}
		delete filter;
	}

	// is this a reply to a continuous service query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        (msg->getMeta("um.rpc.type").compare("discovered") == 0 ||
	         msg->getMeta("um.rpc.type").compare("vanished") == 0)) {
		// _svcQueries comparator uses filter uuid
		ServiceFilter* keyFilter = new ServiceFilter("");
		keyFilter->_uuid = msg->getMeta("um.rpc.filterId");
		if (_localQueries.find(keyFilter) != _localQueries.end()) {
			ResultSet<ServiceDescription>* listener = _localQueries[keyFilter];
			assert(msg->getMeta("um.rpc.channel").size() > 0);
			if (_remoteSvcDesc.find(msg->getMeta("um.rpc.channel")) == _remoteSvcDesc.end()) {
				_remoteSvcDesc[msg->getMeta("um.rpc.channel")] = shared_ptr<ServiceDescription>(new ServiceDescription(msg));
				_remoteSvcDesc[msg->getMeta("um.rpc.channel")]->_svcManager = this;
//				_remoteNodeServices[msg->getMeta("um.pubI")]
			}
			if (msg->getMeta("um.rpc.type").compare("discovered") == 0) {
				listener->added(_remoteSvcDesc[msg->getMeta("um.rpc.channel")]);
			} else {
				listener->removed(_remoteSvcDesc[msg->getMeta("um.rpc.channel")]);
				_remoteSvcDesc.erase(msg->getMeta("um.rpc.channel"));
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
			foundMsg->putMeta("um.rpc.filterId", filterIter->second->_uuid);
			foundMsg->putMeta("um.rpc.type", "discovered");
			foundMsg->putMeta("um.rpc.channel", desc->getChannelName());
			foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
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
			removeMsg->putMeta("um.rpc.filterId", filterIter->second->_uuid);
			removeMsg->putMeta("um.rpc.type", "vanished");
			removeMsg->putMeta("um.rpc.channel", desc->getChannelName());
			removeMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
			_svcPub->send(removeMsg);
			delete removeMsg;
		}
		filterIter++;
	}

	_svc.erase(svcPtr);
	_localSvcDesc.erase(svcPtr);

}

}