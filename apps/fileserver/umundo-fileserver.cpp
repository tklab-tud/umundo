#include <umundo/core.h>
#include <umundo/rpc.h>
#include <protobuf/generated/DirectoryListingService.rpc.pb.h>
#include "umundo/filesystem/DirectoryListingService.h"

using namespace std;
using namespace umundo;

#ifdef WIN32
#include "XGetopt.h"
#endif

#define VERSION "0.0.2"
static const char* progName = "umundo-fileserver";

class PatternMatchingDirectoryListingService : public DirectoryListingService {
public:
	PatternMatchingDirectoryListingService(const string& dir, Regex re) : DirectoryListingService(dir), _re(re) {}
	bool filter(const string& filename) {
		if(_re.matches(filename)) {
			cout << "Serving " << filename << endl;
			return true;
		} else {
			cout << "Not serving " << filename << endl;
			return false;
		}
	}

	Regex _re;
};

void printUsageAndExit() {
	printf("%s version " VERSION "\n", progName);
	printf("Usage: ");
	printf("%s [-d domain] [-p pattern] -f directory\n", progName);
	printf("\n");
	printf("Options\n");
	printf("\t-d <domain>        : domain to join\n");
	printf("\t-p <pattern>       : the regular expression to match files against\n");
	printf("\t-f <directory>     : the directory with the files to serve\n");
	exit(EXIT_FAILURE);
}

string dir;
Regex re(".*");

int main(int argc, char** argv, char** envp) {
	int option;
	while ((option = getopt(argc, argv, "d:p:f:")) != -1) {
		switch(option) {
		case 'f':
			dir = optarg;
			break;
		case 'p':
			re.setPattern(optarg);
			if (re.hasError()) {
				std::cerr << "Can not parse pattern '" << optarg << "' into regular expression" << std::endl;
				exit(EXIT_FAILURE);
			}
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (!(dir.length() > 0))
		printUsageAndExit();

	std::cout << "Serving files matching " << re.getPattern() << "' in " << dir << std::endl;

	// create instances
	Node* node = new Node();
	ServiceManager* svcMgr = new ServiceManager();
	PatternMatchingDirectoryListingService* dirListSvc = new PatternMatchingDirectoryListingService(dir, re);
	ServiceDescription* svcDesc = new ServiceDescription("DirectoryListingService");
	svcDesc->setProperty("dir", dir);
	svcDesc->setProperty("pattern", re.getPattern());
	svcDesc->setProperty("hostId", Host::getHostId());

	// assemble object model
	svcMgr->addService(dirListSvc, svcDesc);
	node->connect(svcMgr);

	std::cout << "Press CTRL+C to end" << std::endl;

	while(true)
		Thread::sleepMs(1000);

	return EXIT_SUCCESS;
}