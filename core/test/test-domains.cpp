#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024*1024

using namespace umundo;

static int receives = 0;
static Mutex mutex;
static string hostId;


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
	Node* fooNode = new Node(hostId + "foo");
	Node* barNode = new Node(hostId + "bar");
	printf("%d\n", Node::instances);
	assert(Node::instances == 2);

	Subscriber* sub = new Subscriber("test1", new TestReceiver("test1"));
	Publisher* pub = new Publisher("test1");

	fooNode->addPublisher(pub);
	barNode->addSubscriber(sub);
	Thread::sleepMs(1000);
	assert(pub->waitForSubscribers(0) == 0);

	delete(fooNode);
	delete(barNode);

	delete(sub);
	delete(pub);
}

void testSameDomain() {
	Node* fooNode = new Node(hostId + "foo");
	Node* barNode = new Node(hostId + "foo");
	assert(Node::instances == 2);

	Subscriber* sub = new Subscriber("test1", new TestReceiver("test1"));
	Publisher* pub = new Publisher("test1");

	fooNode->addPublisher(pub);
	barNode->addSubscriber(sub);
	assert(pub->waitForSubscribers(1) >= 1);

	delete(fooNode);
	delete(barNode);

	delete(sub);
	delete(pub);
}

void testDomainReception() {
	Node* fooNode1 = new Node(hostId + "foo");
	Node* fooNode2 = new Node(hostId + "foo");
	Node* barNode = new Node(hostId + "bar");
	assert(Node::instances == 3);

	Subscriber* sub = new Subscriber("test1", new TestReceiver("test1"));
	Publisher* pub = new Publisher("test1");

	fooNode1->addPublisher(pub);
	fooNode2->addPublisher(pub);
	barNode->addPublisher(pub);

	fooNode1->addSubscriber(sub);
	fooNode2->addSubscriber(sub);
	barNode->addSubscriber(sub);

	char buffer[BUFFER_SIZE];
	for (int i = 0; i < BUFFER_SIZE; i++) {
		buffer[i] = (char)i%255;
	}

	pub->waitForSubscribers(1);
	assert(pub->waitForSubscribers(1) >= 1);
	Thread::sleepMs(100);

	int iterations = 10; // this has to be less or equal to the high water mark / 3
	receives = 0;
	for (int i = 0; i < iterations; i++) {
		Message* msg = new Message();
		msg->setData(buffer, BUFFER_SIZE);
		msg->putMeta("type", "foo!");
		pub->send(msg);
		delete(msg);
	}

	Thread::sleepMs(200);
	std::cout << "Received " << receives << " messages, expected " << iterations << " messages" << std::endl;
//  assert(receives == iterations);

	delete(fooNode1);
	delete(fooNode2);
	delete(barNode);

	delete(sub);
	delete(pub);

}

int main(int argc, char** argv, char** envp) {
	hostId = Host::getHostId();
//	setenv("UMUNDO_LOGLEVEL_DISC", "4", 1);
//	setenv("UMUNDO_LOGLEVEL_NET", "2", 1);
	int i = 1;
	while(i-- > 0) {
//		printf("##### testDifferentDomain #####################################\n");
//		testDifferentDomain();
//		assert(Node::instances == 0);
		printf("##### testSameDomain ##########################################\n");
		testSameDomain();
		assert(Node::instances == 0);
		printf("##### testDomainReception #####################################\n");
		testDomainReception();
		assert(Node::instances == 0);
	}
	return EXIT_SUCCESS;
}
