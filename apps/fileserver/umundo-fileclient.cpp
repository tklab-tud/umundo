#include <umundo/core.h>
#include <umundo/rpc.h>
#include "umundo/filesystem/DirectoryListingClient.h"
#include "umundo/cache/NBandCache.h"

using namespace std;
using namespace umundo;

#ifdef WIN32
#include "XGetopt.h"
#endif

#define VERSION "0.0.2"
static const char* progName = "umundo-fileclient";

static NBandCache nbCache(5);

class DirectoryCacheItem : public NBandCacheItem {
};

/**
 * Monitor individual directory entries.
 */
class CacheInsertingDirectoryEntryMonitor : public ResultSet<DirectoryEntry> {
public:
	CacheInsertingDirectoryEntryMonitor(boost::shared_ptr<ServiceDescription> svcDesc) {
		ScopeLock lock(_mutex);
		_bandName = svcDesc->getChannelName();
		_client = new DirectoryListingClient(svcDesc.get(), this);
		std::vector<boost::shared_ptr<DirectoryEntry> > files = _client->list(".*");
		std::vector<boost::shared_ptr<DirectoryEntry> >::iterator fileIter = files.begin();
		while(fileIter != files.end()) {
			added(*fileIter);
			fileIter++;
		}
	}

	void added(boost::shared_ptr<DirectoryEntry> dirEntry) {
		ScopeLock lock(_mutex);

		cout << "Added " << dirEntry->name() << endl;
	}

	void removed(boost::shared_ptr<DirectoryEntry> dirEntry) {
		ScopeLock lock(_mutex);
		cout << "Removed " << dirEntry->name() << endl;
	}

	void changed(boost::shared_ptr<DirectoryEntry> dirEntry) {
		ScopeLock lock(_mutex);
		cout << "Changed " << dirEntry->name() << endl;
	}

	string _bandName;
	Mutex _mutex;
	std::map<boost::shared_ptr<DirectoryEntry>, DirectoryCacheItem*> _items;
	DirectoryListingClient* _client;
	boost::shared_ptr<ServiceDescription> _svcDesc;
};

/**
 * Monitor DirectoryListingServices.
 */
class DirectoryListingServiceMonitor : public ResultSet<ServiceDescription> {
public:
	DirectoryListingServiceMonitor(ServiceManager* svcMgr) : _svcMgr(svcMgr) {}

	void added(boost::shared_ptr<ServiceDescription> svcDesc) {
		cout << "Added " << svcDesc->getName() << endl;
		if (_dirEntryMonitor.find(svcDesc->getChannelName()) == _dirEntryMonitor.end()) {
			_dirEntryMonitor[svcDesc->getChannelName()] = new CacheInsertingDirectoryEntryMonitor(svcDesc);
		}
	}

	void removed(boost::shared_ptr<ServiceDescription> svcDesc) {
		cout << "Removed " << svcDesc->getName() << endl;
		if (_dirEntryMonitor.find(svcDesc->getChannelName()) != _dirEntryMonitor.end()) {
			_dirEntryMonitor.erase(svcDesc->getChannelName());
		}
	}

	void changed(boost::shared_ptr<ServiceDescription> svcDesc) {
		cout << "Changed " << svcDesc->getName() << endl;
	}

	map<string, CacheInsertingDirectoryEntryMonitor*> _dirEntryMonitor;
	ServiceManager* _svcMgr;
};

void printUsageAndExit() {
	printf("%s version " VERSION "\n", progName);
	printf("Usage: ");
	printf("%s [-d domain]\n", progName);
	printf("\n");
	printf("Options\n");
	printf("\t-d <domain>        : domain to join\n");
	exit(EXIT_FAILURE);
}

string domain;

int main(int argc, char** argv, char** envp) {
	int option;
	while ((option = getopt(argc, argv, "d:p:f:")) != -1) {
		switch(option) {
		case 'd':
			domain = optarg;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	std::cout << "Waiting for DirectoryListingServices" << std::endl;

	// create instances
	Node* node = new Node(domain);
	ServiceManager* svcMgr = new ServiceManager();

	ServiceFilter* svcFilter = new ServiceFilter("DirectoryListingService");
	svcFilter->addRule("hostId", Host::getHostId());

	// register DirectoryListingServiceMonitor
	DirectoryListingServiceMonitor* dirMon = new DirectoryListingServiceMonitor(svcMgr);

	svcMgr->startQuery(svcFilter, dirMon);
	node->connect(svcMgr);

	std::cout << "Press CTRL+C to end" << std::endl;

	while(true)
		Thread::sleepMs(1000);

	return EXIT_SUCCESS;
}
