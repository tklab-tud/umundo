#include <umundo/core.h>
#include <umundo/s11n.h>
#define protected public
#include <umundo/rpc.h>
#undef protected

#include "protobuf/generated/TestServices.rpc.pb.h"
#include "protobuf/generated/TestServices.pb.h"

using namespace umundo;

#define BUFFER_SIZE 1024 * 1024
static string hostId;

class EchoService : public EchoServiceBase {
public:
	EchoReply* echo(EchoRequest* req) {
		EchoReply* reply = new EchoReply();
		reply->set_name(req->name());
		reply->set_buffer(req->buffer().c_str(), BUFFER_SIZE);
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

bool findServices() {
	char* buffer = (char*)malloc(BUFFER_SIZE);
	for (int i = 0; i < BUFFER_SIZE; i++)
		buffer[i] = (char)i + 1;

	for (int i = 0; i < 2; i++) {
		LOG_INFO("Instantiating nodes");
		Node* node1 = new Node(hostId);
		Node* node2 = new Node(hostId);

		LOG_INFO("Instantiating local services");
		EchoService* localEchoService = new EchoService();
		PingService* localPingService = new PingService();

		LOG_INFO("Connecting ServiceManagers");
		ServiceManager* svcMgr1 = new ServiceManager();
		node1->connect(svcMgr1);

		ServiceManager* svcMgr2 = new ServiceManager();
		node2->connect(svcMgr2);

		LOG_INFO("Adding Services to ServiceManager1");
		svcMgr1->addService(localEchoService);
		svcMgr1->addService(localPingService);

		LOG_INFO("Querying from ServiceManager2 for a PingService");
		ServiceFilter* pingSvcFilter = new ServiceFilter("PingService");
		ServiceDescription* pingSvcDesc = svcMgr2->find(pingSvcFilter);
		assert(pingSvcDesc);
		PingServiceStub* pingSvc = new PingServiceStub(pingSvcDesc);
		delete pingSvcFilter;

		int iterations = 5;
		int sends = 0;

		time_t now;
		time_t start;
		time(&start);

		while (iterations > 0) {

			PingRequest* pingReq = new PingRequest();
			pingReq->set_name("ping");
			PingReply* pingRep = pingSvc->ping(pingReq);
			assert(pingRep->name().compare("pong") == 0);

			sends++;
			time(&now);
			if ((now - start) > 0) {
				iterations--;
				std::cout << sends << " requests per second " << iterations << " iterations remaining" << std::endl;
				time(&start);
				sends = 0;
			}

			delete pingReq;
			delete pingRep;

		}
		// test rpc throughput with the echo service
		iterations = 5;
		sends = 0;

		ServiceFilter* echoSvcFilter = new ServiceFilter("EchoService");
		EchoServiceStub* echoSvc = new EchoServiceStub(svcMgr2->find(echoSvcFilter));
		time(&start);
		while (iterations > 0) {
			EchoRequest* echoReq = new EchoRequest();
			echoReq->set_name(".");
			echoReq->set_buffer(buffer, BUFFER_SIZE);

			EchoReply* echoRep = echoSvc->echo(echoReq);
			assert(echoRep->name().compare(".") == 0);
			const char* repBuffer = echoRep->buffer().c_str();
			assert(memcmp(repBuffer, buffer, BUFFER_SIZE) == 0);
			(void)repBuffer;

			sends++;
			time(&now);
			if ((now - start) > 0) {
				iterations--;
				std::cout << sends << " requests per second " << iterations << " iterations remaining" << std::endl;
				time(&start);
				sends = 0;
			}
			delete echoReq;
			delete echoRep;
		}

		svcMgr1->removeService(localEchoService);
		svcMgr1->removeService(localPingService);
		delete localEchoService;
		delete localPingService;

		node1->disconnect(svcMgr1);
		node2->disconnect(svcMgr2);
		delete svcMgr1;
		delete svcMgr2;

		delete node1;
		delete node2;

	}
	return true;
}

bool queryTests() {
	ServiceDescription* desc = new ServiceDescription(map<string, string>());
  desc->_svcName = "FooService";
	desc->setProperty("foo", "the lazy brown fox 123.34");

	ServiceFilter* filter = new ServiceFilter("FooService");

	// whole value is target string
	filter->addRule("foo", "the lazy brown fox 123.34");
	assert(filter->matches(desc));

	// Numbers / OP_EQUALS ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "123.34", ServiceFilter::OP_EQUALS);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "123.34", ServiceFilter::MOD_NOT | ServiceFilter::OP_EQUALS);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "23", ServiceFilter::MOD_NOT | ServiceFilter::OP_EQUALS);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "23", ServiceFilter::OP_EQUALS);
	assert(!filter->matches(desc));

	// Numbers / OP_GREATER ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "23.34", ServiceFilter::OP_GREATER);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "23.34", ServiceFilter::MOD_NOT | ServiceFilter::OP_GREATER);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "323", ServiceFilter::MOD_NOT | ServiceFilter::OP_GREATER);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "323", ServiceFilter::OP_GREATER);
	assert(!filter->matches(desc));

	// Numbers / OP_LESS ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "323.34", ServiceFilter::OP_LESS);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "323.34", ServiceFilter::MOD_NOT | ServiceFilter::OP_LESS);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "123", ServiceFilter::MOD_NOT | ServiceFilter::OP_LESS);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "brown fox (\\d+(\\.\\d+)?)", "123", ServiceFilter::OP_LESS);
	assert(!filter->matches(desc));

	// Strings / OP_EQUALS ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy)", "lazy", ServiceFilter::OP_EQUALS);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy)", "lazy", ServiceFilter::MOD_NOT | ServiceFilter::OP_EQUALS);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "brown (fox)", "dog", ServiceFilter::MOD_NOT | ServiceFilter::OP_EQUALS);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "brown (fox)", "dog", ServiceFilter::OP_EQUALS);
	assert(!filter->matches(desc));

	// Strings / OP_STARTS_WITH ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "lazy", ServiceFilter::OP_STARTS_WITH);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy brown)", "lazy", ServiceFilter::MOD_NOT | ServiceFilter::OP_STARTS_WITH);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "clever", ServiceFilter::MOD_NOT | ServiceFilter::OP_STARTS_WITH);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "the (lazy brown)", "clever", ServiceFilter::OP_STARTS_WITH);
	assert(!filter->matches(desc));

	// Strings / OP_ENDS_WITH ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "brown", ServiceFilter::OP_ENDS_WITH);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy brown)", "brown", ServiceFilter::MOD_NOT | ServiceFilter::OP_ENDS_WITH);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "black", ServiceFilter::MOD_NOT | ServiceFilter::OP_ENDS_WITH);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "the (lazy brown)", "black", ServiceFilter::OP_ENDS_WITH);
	assert(!filter->matches(desc));

	// Strings / OP_CONTAINS ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "y b", ServiceFilter::OP_CONTAINS);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy brown)", "y b", ServiceFilter::MOD_NOT | ServiceFilter::OP_CONTAINS);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "z b", ServiceFilter::MOD_NOT | ServiceFilter::OP_CONTAINS);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "the (lazy brown)", "z b", ServiceFilter::OP_CONTAINS);
	assert(!filter->matches(desc));

	// Strings / OP_GREATER ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "k", ServiceFilter::OP_GREATER);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy brown)", "k", ServiceFilter::MOD_NOT | ServiceFilter::OP_GREATER);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "m", ServiceFilter::MOD_NOT | ServiceFilter::OP_GREATER);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "the (lazy brown)", "m", ServiceFilter::OP_GREATER);
	assert(!filter->matches(desc));

	// Strings / OP_LESS ------------------------------------
	filter->clearRules();
	// equal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "m", ServiceFilter::OP_LESS);
	assert(filter->matches(desc));

	// equal but not supposed to be
	filter->addRule("foo", "the (lazy brown)", "m", ServiceFilter::MOD_NOT | ServiceFilter::OP_LESS);
	assert(!filter->matches(desc));

	filter->clearRules();
	// unequal and supposed to be
	filter->addRule("foo", "the (lazy brown)", "k", ServiceFilter::MOD_NOT | ServiceFilter::OP_LESS);
	assert(filter->matches(desc));

	filter->clearRules();
	// unequal but supposed to be
	filter->addRule("foo", "the (lazy brown)", "k", ServiceFilter::OP_LESS);
	assert(!filter->matches(desc));

	return true;
}

class ServiceListener : public ResultSet<ServiceDescription> {
public:
	virtual ~ServiceListener() {}

	virtual void added(shared_ptr<ServiceDescription> desc) {
		std::cout << "Found Service" << std::endl;
		_instances.insert(desc);
	}

	virtual void removed(shared_ptr<ServiceDescription> desc) {
		std::cout << "Lost Service" << std::endl;
		_instances.erase(desc);
	}

	virtual void changed(shared_ptr<ServiceDescription> desc) {}

	set<shared_ptr<ServiceDescription> > _instances;
};

bool continuousQueries() {
	int iterations = 5;
	Node* hostNode = new Node(hostId);
	Node* queryNode = new Node(hostId);

	while (iterations) {
		std::cout << "Starting continuous query test" << std::endl;

		PingService* localPingService1 = new PingService();
		PingService* localPingService2 = new PingService();
		PingService* localPingService3 = new PingService();

		ServiceManager* hostMgr = new ServiceManager();
		hostNode->connect(hostMgr);
		hostMgr->addService(localPingService1);

		// service query with a late arriving service manager -> greeter welcome
		ServiceFilter* pingSvcFilter = new ServiceFilter("PingService");
		ServiceListener* svcListener = new ServiceListener();

		ServiceManager* queryMgr = new ServiceManager();
		queryNode->connect(queryMgr);
		queryMgr->startQuery(pingSvcFilter, svcListener);
		std::cout << "\tDone Setup nodes and services" << std::endl;

		Thread::sleepMs(2000);
		assert(svcListener->_instances.size() == 1);
		std::cout << "\tFound first service" << std::endl;

		// adding another matching service
		hostMgr->addService(localPingService2);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tFound second service" << std::endl;

		// adding the same service ought to be ignored
		hostMgr->addService(localPingService2);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tNo change with duplicate service" << std::endl;

		// adding another matching service
		hostMgr->addService(localPingService3);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 3);
		std::cout << "\tFound third service" << std::endl;

		// remove matching services
		hostMgr->removeService(localPingService1);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tService removed sucessfully" << std::endl;

		// removing same service ought to do nothing
		hostMgr->removeService(localPingService1);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tNo change with duplicate removal" << std::endl;

		hostMgr->removeService(localPingService2);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 1);
		std::cout << "\tService removed sucessfully" << std::endl;

		// add service again
		hostMgr->addService(localPingService2);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tFound second service again" << std::endl;

		// stop query - listener should know nothing
		queryMgr->stopQuery(pingSvcFilter);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tQuery removed" << std::endl;

		queryMgr->startQuery(pingSvcFilter, svcListener);
		Thread::sleepMs(200);
		assert(svcListener->_instances.size() == 2);
		std::cout << "\tQuery restarted" << std::endl;

		hostNode->disconnect(hostMgr);
		Thread::sleepMs(400);
		assert(svcListener->_instances.size() == 0);
		std::cout << "\tService Managers disconnected" << std::endl;

		queryNode->disconnect(queryMgr);
		delete svcListener;

		delete queryMgr;
		delete hostMgr;

		delete localPingService1;
		delete localPingService2;

		delete pingSvcFilter;
		std::cout << "\tInstances deleted" << std::endl;

		iterations--;
	}
	return true;
}

int main(int argc, char** argv) {
	hostId = Host::getHostId();
	if (!findServices())
		return EXIT_FAILURE;
	if (!queryTests())
		return EXIT_FAILURE;
	if (!continuousQueries())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}