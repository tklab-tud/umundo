#include <umundo/core.h>
#include <umundo/s11n.h>
#include <umundo/rpc.h>

#include "protobuf/generated/TestServices.rpc.pb.h"
#include "protobuf/generated/TestServices.pb.h"

using namespace umundo;

class EchoService : public EchoServiceBase {
public:
	EchoReply* echo(EchoRequest* req) {
		std::cout << "Echoing " << req->name() << std::endl;
		EchoReply* reply = new EchoReply();
		reply->set_name(req->name());
		reply->set_buffer(req->buffer().c_str(), req->buffer().length());
		return reply;
	}
};

int main(int argc, char** argv) {
	Node* n = new Node();
	ServiceManager* svcMgr= new ServiceManager();

	ServiceDescription* echoSvcDesc = new ServiceDescription();

	EchoService* echoSvc = new EchoService();
	svcMgr->addService(echoSvc, echoSvcDesc);

	n->connect(svcMgr);
	while(true)
		Thread::sleepMs(1000);
}