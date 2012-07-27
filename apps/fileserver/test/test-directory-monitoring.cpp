#include "umundo/core.h"
#include "umundo/rpc.h"
#include "umundo/util.h"
#include "umundo/filesystem/DirectoryListingService.h"
#include "umundo/filesystem/DirectoryListingClient.h"

#include <iostream>
#include <stdio.h>
#include <vector>

using namespace umundo;

class DirectoryListener : public ResultSet<DirectoryEntry> {
	void added(shared_ptr<DirectoryEntry> entry) {
		std::cout << "Added " << entry->name() << std::endl;
	}
	void removed(shared_ptr<DirectoryEntry> entry) {
		std::cout << "Removed " << entry->name() << std::endl;
	}
	void changed(shared_ptr<DirectoryEntry> entry) {
		std::cout << "Changed " << entry->name() << std::endl;
	}
};

int main(int argc, char** argv, char** envp) {
	Node* hostNode = new Node();
	Node* queryNode = new Node();

	ServiceManager* hostMgr = new ServiceManager();
	DirectoryListingService* dirListSvc = new DirectoryListingService(".");
	hostMgr->addService(dirListSvc);
	hostNode->connect(hostMgr);

	ServiceManager* queryMgr = new ServiceManager();
	queryNode->connect(queryMgr);

	ServiceFilter* dirListFilter = new ServiceFilter("DirectoryListingService");
	ServiceDescription* dirListDesc = queryMgr->find(dirListFilter);
	DirectoryListingClient* dirStub = new DirectoryListingClient(dirListDesc, new DirectoryListener());

	DirectoryListingRequest* listReq = new DirectoryListingRequest();
	listReq->set_pattern(".*");

	std::vector<shared_ptr<DirectoryEntry> > listRep = dirStub->list(".*");
	std::cout << "Got info on " << listRep.size() << " directory entries" << std::endl;
	for (unsigned int i = 0; i < listRep.size(); i++) {
		shared_ptr<DirectoryEntry> dirEntry = listRep[i];
		DirectoryEntryContent* content = dirStub->get(dirEntry.get());

		std::cout << dirEntry->name() << std::endl;
		std::cout << "\tchecksum: " << content->md5() << std::endl;
		std::cout << "\tsize: " << dirEntry->size() << std::endl;
		std::cout << "\tatime: " << dirEntry->atime_ms() << std::endl;
		std::cout << "\tbtime: " << dirEntry->btime_ms() << std::endl;
		std::cout << "\tctime: " << dirEntry->ctime_ms() << std::endl;
		std::cout << "\tmtime: " << dirEntry->mtime_ms() << std::endl;

		delete content;

	}

	Thread::sleepMs(1000000);
	exit(EXIT_SUCCESS);

}
