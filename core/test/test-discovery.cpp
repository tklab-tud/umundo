#define protected public
#define private public
#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

using namespace umundo;

static int receives = 0;
static string hostId;
static Monitor monitor;
static Mutex mutex;

class TestDiscoverer : public ResultSet<NodeStub> {
public:
	TestDiscoverer() {
	}
	void added(NodeStub node) {
		printf("node added!\n");
		assert(node.getIP().size() >= 7);
		receives++;
		UMUNDO_SIGNAL(monitor);
	}
	void removed(NodeStub node) {

	}
	void changed(NodeStub node) {

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
	(void)testDiscoverer;
	shared_ptr<NodeQuery> query = shared_ptr<NodeQuery>(new NodeQuery(hostId + "fooDomain", testDiscoverer));
	Discovery::browse(query);

	TestDiscoverable* testDiscoverable = new TestDiscoverable(hostId + "fooDomain");
  // ZeroMQNode pimpl constructor already added us to discovery
	//Discovery::add(testDiscoverable);
	while(receives < 1)
		UMUNDO_WAIT(monitor, mutex);
	std::cout << "Successfully found node via discovery" << std::endl;
	return true;
}

bool testPubSubConnections() {
	// test node / publisher / subscriber churn
	for (int i = 0; i < 4; i++) {
		Node node1(hostId);
		Node node2(hostId);
    
    Thread::sleepMs(2000);
    boost::shared_ptr<ZeroMQNode> nodeImpl1 = boost::static_pointer_cast<ZeroMQNode>(node1.getImpl());
    boost::shared_ptr<ZeroMQNode> nodeImpl2 = boost::static_pointer_cast<ZeroMQNode>(node2.getImpl());
    
    assert(nodeImpl1->_nodes.find(nodeImpl2->getUUID()) != nodeImpl1->_nodes.end());
    assert(nodeImpl2->_nodes.find(nodeImpl1->getUUID()) != nodeImpl2->_nodes.end());
    
		for (int j = 0; j < 2; j++) {
			Subscriber sub1("foo", NULL);
			Subscriber sub2("foo", NULL);
			Subscriber sub3("foo", NULL);
			Publisher pub("foo");

			int subs = 0;
			(void)subs;
			node1.addPublisher(pub);
			subs = pub.waitForSubscribers(0);
			assert(subs == 0);

      Thread::sleepMs(300);
      // node2 is aware of node1 publisher
      assert(nodeImpl2->_nodes[nodeImpl1->getUUID()].getImpl()->_pubs.find(pub.getUUID()) != nodeImpl2->_nodes[nodeImpl1->getUUID()].getImpl()->_pubs.end());
      
			node2.addSubscriber(sub1);
			std::cout << "Waiting for 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(1);
      Thread::sleepMs(300);
			assert(subs == 1);
      assert(nodeImpl1->_subscriptions.count("foo") == 1);
      assert(nodeImpl1->_subscriptions.count("~" + sub1.getUUID()) == 1);
      assert(nodeImpl1->_subscriptions.size() == 2);
      assert(nodeImpl1->_confirmedSubscribers.find(sub1.getUUID()) != nodeImpl1->_confirmedSubscribers.end());

			node2.addSubscriber(sub2);
			std::cout << "Waiting for 2nd subscriber" << std::endl;
			subs = pub.waitForSubscribers(2);
      Thread::sleepMs(300);
			assert(subs == 2);
      assert(nodeImpl1->_subscriptions.count("foo") == 2);
      assert(nodeImpl1->_subscriptions.count("~" + sub2.getUUID()) == 1);
      assert(nodeImpl1->_subscriptions.size() == 4);
      assert(nodeImpl1->_confirmedSubscribers.find(sub2.getUUID()) != nodeImpl1->_confirmedSubscribers.end());

			node2.addSubscriber(sub3);
			std::cout << "Waiting for 3rd subscriber" << std::endl;
			subs = pub.waitForSubscribers(3);
			assert(subs == 3);
      assert(nodeImpl1->_subscriptions.count("foo") == 3);
      assert(nodeImpl1->_subscriptions.count("~" + sub3.getUUID()) == 1);
      assert(nodeImpl1->_subscriptions.size() == 6);
      assert(nodeImpl1->_confirmedSubscribers.find(sub3.getUUID()) != nodeImpl1->_confirmedSubscribers.end());

			node2.removeSubscriber(sub1);
			Thread::sleepMs(3000);
			std::cout << "Removed 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(0);
			assert(subs == 2);
      assert(nodeImpl1->_subscriptions.count("foo") == 2);
      assert(nodeImpl1->_subscriptions.count(sub1.getUUID()) == 0);
      assert(nodeImpl1->_subscriptions.size() == 4);
      assert(nodeImpl1->_confirmedSubscribers.find(sub1.getUUID()) == nodeImpl1->_confirmedSubscribers.end());


			node2.removeSubscriber(sub2);
			std::cout << "Removed 2nd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 1);
      assert(nodeImpl1->_subscriptions.count("foo") == 1);
      assert(nodeImpl1->_subscriptions.count(sub2.getUUID()) == 0);
      assert(nodeImpl1->_subscriptions.size() == 2);
      assert(nodeImpl1->_confirmedSubscribers.find(sub2.getUUID()) == nodeImpl1->_confirmedSubscribers.end());

			node2.removeSubscriber(sub3);
			std::cout << "Removed 3rd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 0);
      assert(nodeImpl1->_subscriptions.count("foo") == 0);
      assert(nodeImpl1->_subscriptions.count(sub3.getUUID()) == 0);
      assert(nodeImpl1->_subscriptions.size() == 0);
      assert(nodeImpl1->_confirmedSubscribers.find(sub3.getUUID()) == nodeImpl1->_confirmedSubscribers.end());

			node1.removePublisher(pub);

			std::cout << "--- Successfully connected subscribers to publishers" << std::endl;

		}
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
