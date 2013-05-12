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

#include "umundo/filesystem/DirectoryListingClient.h"

namespace umundo {

DirectoryListingClient::DirectoryListingClient(ServiceDescription* desc, ResultSet<DirectoryEntry>* listener) :
	DirectoryListingServiceStub(desc),
	_listener(listener) {
	_notifySub = new TypedSubscriber(desc->getChannelName() + ":notify", this);
	_notifySub->registerType("DirectoryEntry", new DirectoryEntry());

	set<Node*> nodes = desc->getServiceManager()->getNodes();
	set<Node*>::iterator nodeIter = nodes.begin();
	while(nodeIter != nodes.end()) {
		(*nodeIter)->connect(this);
		nodeIter++;
	}
}

DirectoryListingClient::~DirectoryListingClient() {
	delete _notifySub;
}

std::set<umundo::Subscriber*> DirectoryListingClient::getSubscribers() {
	set<Subscriber*> subs = ServiceStub::getSubscribers();
	subs.insert(_notifySub);
	return subs;
}

std::vector<shared_ptr<DirectoryEntry> > DirectoryListingClient::list(const string& pattern) {
	ScopeLock lock(_mutex);
	DirectoryListingRequest* req = new DirectoryListingRequest();
	req->set_pattern(pattern);
	DirectoryListingReply* reply = DirectoryListingServiceStub::list(req);

	std::vector<shared_ptr<DirectoryEntry> > entries;
	for (int i = 0; i < reply->entries_size(); i++) {
		string key = reply->entries(i).path() + ":" + reply->entries(i).name();
		if (_knownEntries.find(key) == _knownEntries.end())
			_knownEntries[key] = shared_ptr<DirectoryEntry>(new DirectoryEntry(reply->entries(i)));

		entries.push_back(_knownEntries[key]);
	}

	delete req;
	delete reply;

	return entries;
}

void DirectoryListingClient::receive(void* object, Message* msg) {
	ServiceStub::receive(object, msg);

	if (msg->getMeta().find("operation") != msg->getMeta().end()) {
		ScopeLock lock(_mutex);
		string op = msg->getMeta("operation");
		DirectoryEntry* dirEntry = new DirectoryEntry(*(DirectoryEntry*)object);
		string key = dirEntry->path() + ":" + dirEntry->name();
		if (op.compare("added") == 0) {
			if (_knownEntries.find(key) == _knownEntries.end())
				_knownEntries[key] = shared_ptr<DirectoryEntry>(dirEntry);
			if (_listener != NULL)
				_listener->added(_knownEntries[key]);
		} else if (op.compare("modified") == 0) {
			if (_knownEntries.find(key) != _knownEntries.end()) {
				if (_listener != NULL)
					_listener->changed(_knownEntries[key]);
			} else {
				UM_LOG_ERR("Unknown directory entry reported as modified");
			}
		} else if (op.compare("removed") == 0) {
			if (_knownEntries.find(key) != _knownEntries.end()) {
				if (_listener != NULL)
					_listener->removed(_knownEntries[key]);
			} else {
				UM_LOG_ERR("Unknown directory entry reported as removed");
			}
		}
	}
}

}