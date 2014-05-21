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

#ifndef DIRECTORYLISTINGSERVICEIMPL_H_OW9QMITZ
#define DIRECTORYLISTINGSERVICEIMPL_H_OW9QMITZ

#include <sys/stat.h>

#include "umundo/core.h"
#include "umundo/rpc.h"
#include "umundo/util.h"
#include "protobuf/generated/DirectoryListingService.rpc.pb.h"

namespace umundo {

class UMUNDO_API DirectoryListingService : public DirectoryListingServiceBase, public Thread {
public:
	DirectoryListingService(const string& directory);
	virtual ~DirectoryListingService();

	// Connectable interface
	std::set<umundo::Publisher*> getPublishers();

	DirectoryListingReply* list(DirectoryListingRequest*);
	DirectoryEntryContent* get(DirectoryEntry*);

	void run();
	virtual bool filter(const string& filename) {
		return true;
	}

protected:
	void updateEntries();
	char* readFileIntoBuffer(const string&, int);
	DirectoryEntry* statToEntry(const string&, struct stat);

	void notifyNewFile(const string&, struct stat);
	void notifyModifiedFile(const string&, struct stat);
	void notifyRemovedFile(const string&, struct stat);

	TypedPublisher* _notifyPub;

	string _dir;
	time_t _lastChecked;
	Mutex _mutex;
	map<string, struct stat> _knownEntries;

};

}
#endif /* end of include guard: DIRECTORYLISTINGSERVICEIMPL_H_OW9QMITZ */
