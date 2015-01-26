#include <umundo/core.h>
#include <umundo/s11n.h>
#include <umundo/rpc.h>

#include "protobuf/generated/TestServices.rpc.pb.h"
#include "protobuf/generated/TestServices.pb.h"

using namespace umundo;

#define BUFFER_SIZE 1024 * 1024
static std::string hostId;

class EchoService : public EchoServiceBase {
public:
	EchoReply* echo(EchoRequest* req) {
		EchoReply* reply = new EchoReply();
		reply->set_name(req->name());
//		reply->set_buffer(req->buffer().c_str(), BUFFER_SIZE);
		return reply;
	}
};

class PingService : public PingServiceBase {
public:
	PingReply* ping(PingRequest* req) {
		PingReply* reply = new PingReply();
		reply->set_name("pong");
		return reply;
	}
};

int main(int argc, char** argv) {
	Node n;
	ServiceManager svcMgr;

	Discovery disc(Discovery::MDNS);
	disc.add(n);

	// set some random properties to query for
	ServiceDescription echoSvcDesc;
	echoSvcDesc.setProperty("host", Host::getHostId());
	echoSvcDesc.setProperty("someString", "this is some random string with 123 numbers inside");
	echoSvcDesc.setProperty("someNumber", "1");

	EchoService* echoSvc = new EchoService();
	svcMgr.addService(echoSvc, echoSvcDesc);

	PingService* pingSvc = new PingService();
	svcMgr.addService(pingSvc);


	n.connect(&svcMgr);
	while(true)
		Thread::sleepMs(1000);
}