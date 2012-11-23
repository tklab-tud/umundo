#include <umundo/core.h>
#include <umundo/s11n.h>
#include <umundo/rpc.h>

#include "protobuf/generated/TestServices.rpc.pb.h"
#include "protobuf/generated/TestServices.pb.h"

using namespace umundo;

#define BUFFER_SIZE 1024 * 1024
static string hostId;

bool testEchoClient() {
	Node n;
	ServiceManager svcMgr;
	n.connect(&svcMgr);

	Regex re("(\\d+)");
	if (re.matches("this is some random string with 123 numbers inside")) {
		if (re.hasSubMatches()) {
			std::cout << re.getSubMatches()[0] << std::endl;
		} else {
			std::cout << re.getMatch() << std::endl;
		}
	}

	ServiceFilter echoSvcFilter("EchoService");
	echoSvcFilter.addRule("someString", "(\\d+)", "200", ServiceFilter::OP_LESS);
	echoSvcFilter.addRule("someString", "(\\d+)", "123", ServiceFilter::OP_EQUALS);
	echoSvcFilter.addRule("someString", "this is some", ServiceFilter::OP_STARTS_WITH);

	ServiceDescription echSvcDesc = svcMgr.find(echoSvcFilter);
	if (echSvcDesc) {
		EchoServiceStub* echoSvc = new EchoServiceStub(echSvcDesc);
		int iterations = 100;
		while(iterations-- > 0) {
			EchoRequest* echoReq = new EchoRequest();
			echoReq->set_name(".");
			EchoReply* echoRep = echoSvc->echo(echoReq);
			assert(echoRep->name().compare(".") == 0);
			std::cout << ".";
		}
		return true;
	} else {
		return false;
	}
}

int main(int argc, char** argv) {
	hostId = Host::getHostId();
	if (!testEchoClient())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}