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

#ifndef DIRECTORYLISTINGCLIENT_H_M5X8XRLM
#define DIRECTORYLISTINGCLIENT_H_M5X8XRLM

#include "umundo/core.h"
#include "umundo/s11n.h"
#include "umundo/rpc.h"
#include "protobuf/generated/DirectoryListingService.rpc.pb.h"

namespace umundo {

class DLLEXPORT DirectoryListingClient : public DirectoryListingServiceStub {
public:
	DirectoryListingClient(ServiceDescription*, ResultSet<DirectoryEntry>*);
  ~DirectoryListingClient();
  
  void receive(void* object, Message* msg);

  // Connectable interface
	std::set<umundo::Subscriber*> getSubscribers();

  /// Convenience API
  virtual std::vector<shared_ptr<DirectoryEntry> > list(const string& pattern);
  
protected:
  TypedSubscriber* _notifySub;
  ResultSet<DirectoryEntry>* _listener;
  map<string, shared_ptr<DirectoryEntry> > _knownEntries;
  Mutex _mutex;
};

}
#endif /* end of include guard: DIRECTORYLISTINGCLIENT_H_M5X8XRLM */
