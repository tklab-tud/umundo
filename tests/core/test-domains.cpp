#define protected public
#include "umundo/core.h"
#include "umundo/core/discovery/MDNSDiscovery.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024*1024

using namespace umundo;

static int receives = 0;
static RMutex mutex;
static std::string hostId;


class TestReceiver : public Receiver {
public:
	std::string _name;
	TestReceiver(std::string name) : _name(name) {};
	void receive(Message* msg) {
		//std::cout << "received " << msg->getData().size() << "bytes" << std::endl;
//		assert(_name.compare(msg->getMeta("channelName")) == 0);
		UMUNDO_LOCK(mutex);
		// not an atomic operation!
		receives++;
		UMUNDO_UNLOCK(mutex);
	}
};

void testDifferentDomain() {
	Discovery fooDisc(Discovery::MDNS, "foo.local.");
	Discovery barDisc(Discovery::MDNS, "bar.local.");

	Node fooNode;
	Node barNode;
	assert(NodeImpl::instances == 2);

	fooDisc.add(fooNode);
	barDisc.add(barNode);

	Subscriber sub("test1");
	TestReceiver testRcv("test1");
	sub.setReceiver(&testRcv);

	Publisher pub("test1");

	fooNode.addPublisher(pub);
	barNode.addSubscriber(sub);
	Thread::sleepMs(1000);
	assert(pub.waitForSubscribers(0) == 0);

}

void testSameDomain() {
	Discovery foo1Disc(Discovery::MDNS, "foo.local.");
	Discovery foo2Disc(Discovery::MDNS, "foo.local.");

	Node fooNode;
	Node barNode;
	assert(NodeImpl::instances == 2);

	foo1Disc.add(fooNode);
	foo2Disc.add(barNode);

	Subscriber sub("test1");
	TestReceiver testRcv("test1");
	sub.setReceiver(&testRcv);

	Publisher pub("test1");

	fooNode.addPublisher(pub);
	barNode.addSubscriber(sub);
	assert(pub.waitForSubscribers(1) >= 1);
}

void testDomainReception() {

	Discovery fooDisc(Discovery::MDNS, "foo.local.");
	Discovery barDisc(Discovery::MDNS, "bar.local.");

	Node fooNode1;
	Node fooNode2;
	Node barNode;
	assert(NodeImpl::instances == 3);

	fooDisc.add(fooNode1);
	fooDisc.add(fooNode2);
	barDisc.add(barNode);

	Subscriber sub("test1");
	Publisher pub("test1");

	TestReceiver testRcv("test1");
	sub.setReceiver(&testRcv);
	
	fooNode1.addPublisher(pub);
	fooNode2.addPublisher(pub);
	barNode.addPublisher(pub);

	fooNode1.addSubscriber(sub);
	fooNode2.addSubscriber(sub);
	barNode.addSubscriber(sub);

	char buffer[BUFFER_SIZE];
	for (int i = 0; i < BUFFER_SIZE; i++) {
		buffer[i] = (char)i%255;
	}

	pub.waitForSubscribers(1);
	assert(pub.waitForSubscribers(1) >= 1);
	Thread::sleepMs(100);

	int iterations = 10; // this has to be less or equal to the high water mark / 3
	receives = 0;
	for (int i = 0; i < iterations; i++) {
		Message* msg = new Message();
		msg->setData(buffer, BUFFER_SIZE);
		msg->putMeta("type", "foo!");
		pub.send(msg);
		delete(msg);
	}

	Thread::sleepMs(200);
	std::cout << "Received " << receives << " messages, expected " << iterations << " messages" << std::endl;
//  assert(receives == iterations);

}

int main(int argc, char** argv, char** envp) {
	hostId = Host::getHostId();
	setenv("UMUNDO_LOGLEVEL", "4", 1);
//	setenv("UMUNDO_LOGLEVEL_NET", "2", 1);
	int i = 1;
	while(i-- > 0) {
		printf("##### testDifferentDomain #####################################\n");
		testDifferentDomain();
		assert(NodeImpl::instances == 0);
		printf("##### testSameDomain ##########################################\n");
		testSameDomain();
		assert(NodeImpl::instances == 0);
		printf("##### testDomainReception #####################################\n");
		testDomainReception();
		assert(NodeImpl::instances == 0);
	}
	return EXIT_SUCCESS;
}
