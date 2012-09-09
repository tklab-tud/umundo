#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

using namespace umundo;

static int receives = 0;
static string hostId;
static Monitor monitor;
static Mutex mutex;

class TestDiscoverer : public ResultSet<NodeStub> {
public:
	TestDiscoverer() {
	}
	void added(shared_ptr<NodeStub> node) {
		printf("node added!\n");
		assert(node->getIP().size() >= 7);
		receives++;
		UMUNDO_SIGNAL(monitor);
	}
	void removed(shared_ptr<NodeStub> node) {

	}
	void changed(shared_ptr<NodeStub> node) {

	}
};

class TestDiscoverable : public Node {
public:
	TestDiscoverable(string domain) : Node(domain) {
	}
};

bool testNodeDiscovery() {
	hostId = Host::getHostId();
	std::cout << "HostId:" << hostId << std::endl;

	TestDiscoverer* testDiscoverer = new TestDiscoverer();
	shared_ptr<NodeQuery> query = shared_ptr<NodeQuery>(new NodeQuery(hostId + "fooDomain", testDiscoverer));
	Discovery::browse(query);

	TestDiscoverable* testDiscoverable = new TestDiscoverable(hostId + "fooDomain");
	Discovery::add(testDiscoverable);
	while(receives < 1)
		UMUNDO_WAIT(monitor, mutex);
	std::cout << "Successfully found node via discovery" << std::endl;
	return true;
}

bool testPubSubConnections() {
	// test node / publisher / subscriber churn
	for (int i = 0; i < 2; i++) {
		Node* node1 = new Node(hostId);
		Node* node2 = new Node(hostId);
		for (int j = 0; j < 2; j++) {
			Subscriber* sub1 = new Subscriber("foo", NULL);
			Subscriber* sub2 = new Subscriber("foo", NULL);
			Subscriber* sub3 = new Subscriber("foo", NULL);
			Publisher* pub = new Publisher("foo");

			int subs = 0;
			(void)subs;
			node1->addPublisher(pub);
			subs = pub->waitForSubscribers(0);
			assert(subs == 0);

			node2->addSubscriber(sub1);
			std::cout << "Waiting for 1st subscriber" << std::endl;
			subs = pub->waitForSubscribers(1);
			assert(subs == 1);

			node2->addSubscriber(sub2);
			std::cout << "Waiting for 2nd subscriber" << std::endl;
			subs = pub->waitForSubscribers(2);
			assert(subs == 2);

			node2->addSubscriber(sub3);
			std::cout << "Waiting for 3rd subscriber" << std::endl;
			subs = pub->waitForSubscribers(3);
			assert(subs == 3);

			node2->removeSubscriber(sub1);
			Thread::sleepMs(50);
			std::cout << "Removed 1st subscriber" << std::endl;
			subs = pub->waitForSubscribers(0);
			assert(subs == 2);

			node2->removeSubscriber(sub2);
			std::cout << "Removed 2nd subscriber" << std::endl;
			Thread::sleepMs(50);
			subs = pub->waitForSubscribers(0);
			assert(subs == 1);

			node2->removeSubscriber(sub3);
			std::cout << "Removed 3rd subscriber" << std::endl;
			Thread::sleepMs(50);
			subs = pub->waitForSubscribers(0);
			assert(subs == 0);

			std::cout << "Successfully connected subscribers to publishers" << std::endl;

			node2->removeSubscriber(sub1);
			node2->removeSubscriber(sub2);
			node2->removeSubscriber(sub3);
			node1->removePublisher(pub);

			delete sub1;
			delete sub2;
			delete sub3;
			delete pub;
		}
		delete node1;
		delete node2;
	}
	return true;
}

int main(int argc, char** argv, char** envp) {
//	setenv("UMUNDO_LOGLEVEL", "4", 1);
// 	if (!testNodeDiscovery())
// 		return EXIT_FAILURE;
	if (!testPubSubConnections())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;


}
