#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

using namespace umundo;

static int receives = 0;
static Monitor monitor;

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

int main(int argc, char** argv, char** envp) {
	setenv("UMUNDO_LOGLEVEL", "4", 1);

  std::cout << "HostId:" << Host::getHostId() << std::endl;
  
	TestDiscoverer* testDiscoverer = new TestDiscoverer();
	shared_ptr<NodeQuery> query = shared_ptr<NodeQuery>(new NodeQuery("fooDomain", testDiscoverer));
	Discovery::browse(query);

	TestDiscoverable* testDiscoverable = new TestDiscoverable("fooDomain");
	Discovery::add(testDiscoverable);
	while(receives < 1)
		UMUNDO_WAIT(monitor);

	// test node / publisher / subscriber churn
	for (int i = 0; i < 2; i++) {
		Node* node1 = new Node();
		Node* node2 = new Node();
		for (int j = 0; j < 2; j++) {
			Subscriber* sub1 = new Subscriber("foo", NULL);
			Subscriber* sub2 = new Subscriber("foo", NULL);
			Subscriber* sub3 = new Subscriber("foo", NULL);
			Publisher* pub = new Publisher("foo");

			int subs = 0;
			node1->addPublisher(pub);
			subs = pub->waitForSubscribers(0);
			assert(subs == 0);

			node2->addSubscriber(sub1);
			subs = pub->waitForSubscribers(1);
			assert(subs == 1);

			node2->addSubscriber(sub2);
			subs = pub->waitForSubscribers(2);
			assert(subs == 2);

			node2->addSubscriber(sub3);
			subs = pub->waitForSubscribers(3);
			assert(subs == 3);

			node2->removeSubscriber(sub1);
			Thread::sleepMs(50);
			subs = pub->waitForSubscribers(0);
			assert(subs == 2);

			node2->removeSubscriber(sub2);
			Thread::sleepMs(50);
			subs = pub->waitForSubscribers(0);
			assert(subs == 1);

			node2->removeSubscriber(sub3);
			Thread::sleepMs(50);
			subs = pub->waitForSubscribers(0);
			assert(subs == 0);

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
}
