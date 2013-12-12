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
	// connect to well-known service discovery channel
	_svcPub = new Publisher("umundo.sd");
	_svcSub = new Subscriber("umundo.sd", this);
	_svcPub->setGreeter(this);
}

ServiceManager::~ServiceManager() {
	_svcPub->setGreeter(NULL);
	delete _svcPub;
	delete _svcSub;
}

std::map<std::string, Publisher> ServiceManager::getPublishers() {
	std::map<std::string, Publisher> pubs;
	pubs[_svcPub->getUUID()] = *_svcPub;
	return pubs;
}
std::map<std::string, Subscriber> ServiceManager::getSubscribers() {
	std::map<std::string, Subscriber> subs;
	subs[_svcSub->getUUID()] = *_svcSub;
	return subs;
}

/**
 * Another ServiceManager was added.
 *
 * Send all local continuous queries.
 */
void ServiceManager::welcome(const Publisher& pub, const SubscriberStub& subStub) {
	ScopeLock lock(_mutex);
	UM_LOG_INFO("found remote ServiceManager - sending %d queries", _localQueries.size());
	std::map<ServiceFilter, ResultSet<ServiceDescription>*>::iterator queryIter = _localQueries.begin();
	while (queryIter != _localQueries.end()) {
		Message* queryMsg = queryIter->first.toMessage();
		queryMsg->putMeta("um.rpc.type", "startDiscovery");
		queryMsg->putMeta("um.sub", subStub.getUUID());
		queryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
		_svcPub->send(queryMsg);
		delete queryMsg;
		queryIter++;
	}
	if (_pendingMessages.find(subStub.getUUID()) != _pendingMessages.end()) {
		std::list<std::pair<uint64_t, Message*> >::iterator msgIter = _pendingMessages[subStub.getUUID()].begin();
		while(msgIter != _pendingMessages[subStub.getUUID()].end()) {
			_svcPub->send(msgIter->second);
			delete msgIter->second;
		}
		_pendingMessages.erase(subStub.getUUID());
	}
}

/**
 * A ServiceManager was removed.
 */
void ServiceManager::farewell(const Publisher& pub, const SubscriberStub& subStub) {
	ScopeLock lock(_mutex);
	UM_LOG_INFO("removed remote ServiceManager - notifying", _localQueries.size());

	// did this publisher responded to our queries before?
	if (_remoteSvcDesc.find(subStub.getUUID()) != _remoteSvcDesc.end()) {

		// check all local queries if the remote services matched and notify about removal
		std::map<ServiceFilter, ResultSet<ServiceDescription>*>::iterator queryIter = _localQueries.begin();
		while(queryIter != _localQueries.end()) {

			std::map<std::string, ServiceDescription>::iterator remoteSvcIter = _remoteSvcDesc[subStub.getUUID()].begin();
			while(remoteSvcIter != _remoteSvcDesc[subStub.getUUID()].end()) {
				if (queryIter->first.matches(remoteSvcIter->second)) {
					queryIter->second->removed(remoteSvcIter->second);
				}
				remoteSvcIter++;
			}
			queryIter++;
		}
	}
	_remoteSvcDesc.erase(subStub.getUUID());
}


void ServiceManager::addedToNode(Node& node) {
	ScopeLock lock(_mutex);
	std::map<intptr_t, Service*>::iterator svcIter = _svc.begin();
	while(svcIter != _svc.end()) {
		node.connect(svcIter->second);
		svcIter++;
	}
	_nodes.insert(node);
}

void ServiceManager::removedFromNode(Node& node) {
	ScopeLock lock(_mutex);
	if (_nodes.find(node) == _nodes.end())
		return;

	std::map<intptr_t, Service*>::iterator svcIter = _svc.begin();
	while(svcIter != _svc.end()) {
		node.disconnect(svcIter->second);
		svcIter++;
	}
	_nodes.erase(node);
}

/**
 * Start a query for a service.
 *
 * Remember service filter and send query to all connected ServiceManagers.
 */
void ServiceManager::startQuery(const ServiceFilter& filter, ResultSet<ServiceDescription>* listener) {
	ScopeLock lock(_mutex);
	_localQueries[filter] = listener;
	Message* queryMsg = filter.toMessage();
	queryMsg->putMeta("um.rpc.type", "startDiscovery");
	queryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());

	_svcPub->send(queryMsg);
	UM_LOG_INFO("Sending new query to %d ServiceManagers", _svcPub->waitForSubscribers(0));
	delete queryMsg;
}

void ServiceManager::stopQuery(const ServiceFilter& filter) {
	ScopeLock lock(_mutex);
	if (_localQueries.find(filter) != _localQueries.end()) {
		Message* unqueryMsg = filter.toMessage();
		unqueryMsg->putMeta("um.rpc.type", "stopDiscovery");
		unqueryMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
		_svcPub->send(unqueryMsg);
		delete unqueryMsg;
		_localQueries.erase(filter);
	}
}

ServiceDescription ServiceManager::find(const ServiceFilter& svcFilter) {
	return find(svcFilter, 3000);
}

ServiceDescription ServiceManager::find(const ServiceFilter& svcFilter, int timeout) {
	if(_svcPub->waitForSubscribers(1, timeout) < 1) {
		// there is no other ServiceManager yet
		UM_LOG_INFO("Failed to find another ServiceManager");
		return ServiceDescription();
	}

	Message* findMsg = svcFilter.toMessage();
	std::string reqId = UUID::getUUID();
	findMsg->putMeta("um.rpc.type", "discover");
	findMsg->putMeta("um.rpc.reqId", reqId.c_str());
	findMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
//	Thread::sleepMs(1000);
	if(_findRequests.find(reqId) != _findRequests.end())
		UM_LOG_WARN("Find request %s already received", reqId.c_str());
	_findRequests[reqId] = new Monitor();

	_svcPub->send(findMsg);
	delete findMsg;

	ScopeLock lock(_mutex);
	_findRequests[reqId]->wait(_mutex, timeout);
	delete _findRequests[reqId];
	_findRequests.erase(reqId);

	if (_findResponses.find(reqId) == _findResponses.end()) {
		UM_LOG_INFO("Failed to find %s", svcFilter.getServiceName().c_str());
		return NULL;
	}

	// TODO: Remove other replies as they come in!

	Message* foundMsg = _findResponses[reqId];
	assert(foundMsg != NULL);
	ServiceDescription svcDesc(foundMsg);
	svcDesc._svcManager = this;
	_findResponses.erase(reqId);
	delete foundMsg;
	return svcDesc;

}

/**
 * Find local service matching filter.
 */
std::set<ServiceDescription> ServiceManager::findLocal(const ServiceFilter& svcFilter) {
	std::set<ServiceDescription> foundSvcs;
	std::map<intptr_t, ServiceDescription>::iterator svcDescIter = _localSvcDesc.begin();
	while(svcDescIter != _localSvcDesc.end()) {
		if (svcFilter.matches(svcDescIter->second)) {
			foundSvcs.insert(svcDescIter->second);
			UM_LOG_INFO("we have a match for %s", svcFilter.getServiceName().c_str());
		}
		svcDescIter++;
	}
	return foundSvcs;
}

void ServiceManager::receive(Message* msg) {
	ScopeLock lock(_mutex);
	// is this a response for one of our requests?
	if (msg->getMeta().find("um.rpc.respId") != msg->getMeta().end()) {
		std::string respId = msg->getMeta("um.rpc.respId");
		if (_findRequests.find(respId) != _findRequests.end()) {
			// put message into responses and signal waiting thread
			_findResponses[respId] = new Message(*msg);
			_findRequests[respId]->signal();
		}
	}

	// is someone simply asking for a service via find?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("discover") == 0) {
		ServiceFilter filter(msg);
		std::set<ServiceDescription> foundSvcs = findLocal(filter);

		if (foundSvcs.size() > 0) {
			ServiceDescription svcDesc = (*(foundSvcs.begin()));
			Message* foundMsg = svcDesc.toMessage();
			foundMsg->setReceiver(msg->getMeta("um.rpc.mgrId"));
			foundMsg->putMeta("um.rpc.respId", msg->getMeta("um.rpc.reqId"));
			foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
//			if (_svcPub->isPublishingTo(msg->getMeta("um.rpc.mgrId"))) {
			_svcPub->send(foundMsg);
			// } else {
			// 	// queue message and send in welcome
			// 	_pendingMessages[msg->getMeta("um.rpc.mgrId")].push_back(std::make_pair(Thread::getTimeStampMs(), foundMsg));
			// }
			delete foundMsg;
		}
	}

	// is this the start of a continuous query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("startDiscovery") == 0) {
		ServiceFilter filter(msg);
		_remoteQueries[filter.getUUID()] = filter;

		UM_LOG_INFO("Received query for '%s'", filter._svcName.c_str());

		// do we have such a service?
		std::set<ServiceDescription> foundSvcs = findLocal(filter);
		std::set<ServiceDescription>::iterator svcDescIter = foundSvcs.begin();
		while(svcDescIter != foundSvcs.end()) {
			Message* foundMsg = svcDescIter->toMessage();
			foundMsg->setReceiver(msg->getMeta("um.rpc.mgrId"));
			foundMsg->putMeta("um.rpc.filterId", filter.getUUID());
			foundMsg->putMeta("um.rpc.type", "discovered");
			foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
			_svcPub->send(foundMsg);
			delete foundMsg;
			svcDescIter++;
		}
	}

	// is this the end of a continuous query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        msg->getMeta("um.rpc.type").compare("stopDiscovery") == 0) {
		ServiceFilter filter(msg);
		if (_remoteQueries.find(filter.getUUID()) != _remoteQueries.end()) {
			_remoteQueries.erase(filter.getUUID());
		}
	}

	// is this a reply to a continuous service query?
	if (msg->getMeta().find("um.rpc.type") != msg->getMeta().end() &&
	        (msg->getMeta("um.rpc.type").compare("discovered") == 0 ||
	         msg->getMeta("um.rpc.type").compare("vanished") == 0)) {
		// _svcQueries comparator uses filter uuid
		ServiceFilter keyFilter("");
		keyFilter._uuid = msg->getMeta("um.rpc.filterId");
		if (_localQueries.find(keyFilter) != _localQueries.end()) {
			ResultSet<ServiceDescription>* listener = _localQueries[keyFilter];
			assert(msg->getMeta("um.rpc.desc.channel").size() > 0);
			assert(msg->getMeta("um.rpc.mgrId").size() > 0);
			std::string svcChannel = msg->getMeta("um.rpc.desc.channel");
			std::string managerId = msg->getMeta("um.rpc.mgrId");
			if (_remoteSvcDesc.find(managerId) == _remoteSvcDesc.end() || _remoteSvcDesc[managerId].find(svcChannel) == _remoteSvcDesc[managerId].end()) {
				_remoteSvcDesc[managerId][svcChannel] = ServiceDescription(msg);
				_remoteSvcDesc[managerId][svcChannel]._svcManager = this;
			}
			assert(_remoteSvcDesc.find(managerId) != _remoteSvcDesc.end());
			assert(_remoteSvcDesc[managerId].find(svcChannel) != _remoteSvcDesc[managerId].end());
			if (msg->getMeta("um.rpc.type").compare("discovered") == 0) {
				listener->added(_remoteSvcDesc[managerId][svcChannel]);
			} else {
				listener->removed(_remoteSvcDesc[managerId][svcChannel]);
				_remoteSvcDesc[managerId].erase(svcChannel);
			}
		}
	}
}

void ServiceManager::addService(Service* service) {
	ServiceDescription desc;
	addService(service, desc);
}

void ServiceManager::addService(Service* service, ServiceDescription& desc) {
	ScopeLock lock(_mutex);
	if (service == NULL)
		return;

	intptr_t svcPtr = (intptr_t)service;
	if (_svc.find(svcPtr) != _svc.end())
		return;

	desc._channelName = service->getChannelName();
	desc._svcName = service->getName();

	assert(desc._channelName.length() > 0);

	_svc[svcPtr] = service;
	_localSvcDesc[svcPtr] = desc;

	// connect service to all nodes the manager is connected to
	std::set<Node>::iterator nodeIter;
	nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		((Node)*nodeIter).connect(service);
		nodeIter++;
	}

	// iterate continuous queries and notify other service managers about matches.
	std::map<std::string, ServiceFilter>::iterator filterIter = _remoteQueries.begin();
	while(filterIter != _remoteQueries.end()) {
		if (filterIter->second.matches(desc)) {
			Message* foundMsg = desc.toMessage();
			foundMsg->putMeta("um.rpc.filterId", filterIter->second._uuid);
			foundMsg->putMeta("um.rpc.type", "discovered");
			foundMsg->putMeta("um.rpc.channel", desc.getChannelName());
			foundMsg->putMeta("um.rpc.mgrId", _svcSub->getUUID());
			_svcPub->send(foundMsg);
			delete foundMsg;
		}
		filterIter++;
	}
}

void ServiceManager::removeService(Service* service) {
	ScopeLock lock(_mutex);

	if (service == NULL)
		return;

	intptr_t svcPtr = (intptr_t)service;
	if (_svc.find(svcPtr) == _svc.end())
		return;

	std::set<Node>::iterator nodeIter = _nodes.begin();
	while(nodeIter != _nodes.end()) {
		((Node)*nodeIter).disconnect(service);
		nodeIter++;
	}

	assert(_localSvcDesc.find(svcPtr)!= _localSvcDesc.end());
	ServiceDescription desc = _localSvcDesc[svcPtr];

	// iterate continuous queries and notify other service managers about removals.
	std::map<std::string, ServiceFilter>::iterator filterIter = _remoteQueries.begin();
	while(filterIter != _remoteQueries.end()) {
		if (filterIter->second.matches(desc)) {
			Message* removeMsg = desc.toMessage();
			removeMsg->putMeta("um.rpc.filterId", filterIter->second._uuid);
			removeMsg->putMeta("um.rpc.type", "vanished");
			removeMsg->putMeta("um.rpc.channel", desc.getChannelName());
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