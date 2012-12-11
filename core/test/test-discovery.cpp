#define protected public
#define private public
#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

using namespace umundo;

static string hostId;
static Monitor monitor;
static Mutex mutex;

bool testNodeDiscovery() {
	hostId = Host::getHostId();
	int iterations = 2;
	for (int i = 0; i < iterations; ++i) {
		std::cout << "### instantiating " << i << " nodes" << std::endl;
		std::set<Node> nodes;
		for (int j = 0; j < i; ++j) {
			nodes.insert(Node(hostId));
		}
		std::cout << "--- checking for discovery of " << i - 1 << " nodes" << std::endl;
		std::set<Node>::iterator nodeIter1 = nodes.begin();
		while(nodeIter1 != nodes.end()) {
			boost::shared_ptr<ZeroMQNode> nodeImpl = boost::static_pointer_cast<ZeroMQNode>(nodeIter1->getImpl());
			std::set<Node>::iterator nodeIter2 = nodes.begin();
			while(nodeIter2 != nodes.end()) {
				// make sure everyone found exactly everyone else
				int retries = 50;
				if (nodeIter1 != nodeIter2) {
					while(nodeImpl->_nodes.find(nodeIter2->getUUID()) == nodeImpl->_nodes.end()) {
						Thread::sleepMs(100);
						if (retries-- == 0)
							assert(false);
					}
				}
				nodeIter2++;
			}
			assert(nodeImpl->_nodes.size() == i - 1);
			nodeIter1++;
		}
	}

	return true;
}

bool testNodeRemoval() {
	hostId = Host::getHostId();
	Node node1(hostId);
	boost::shared_ptr<ZeroMQNode> nodeImpl1 = boost::static_pointer_cast<ZeroMQNode>(node1.getImpl());

  int iterations = 2;
	for (int i = 0; i < iterations; i++) {
		Node node2(hostId);
		boost::shared_ptr<ZeroMQNode> nodeImpl2 = boost::static_pointer_cast<ZeroMQNode>(node2.getImpl());

		int retries = 100;
		while(
		    nodeImpl1->_nodes.find(nodeImpl2->getUUID()) == nodeImpl1->_nodes.end() ||
		    nodeImpl2->_nodes.find(nodeImpl1->getUUID()) == nodeImpl2->_nodes.end()) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
		}

//		assert(nodeImpl1->_nodes.size() < iterations / 2);
//		assert(nodeImpl2->_nodes.size() < iterations / 2);
	}

	return true;
}

bool testPubSubConnections() {
	// test node / publisher / subscriber churn
	for (int i = 0; i < 2; i++) {
//		Thread::sleepMs(2000);
		Node node1(hostId);
		Node node2(hostId);

//		Thread::sleepMs(2000);
		boost::shared_ptr<ZeroMQNode> nodeImpl1 = boost::static_pointer_cast<ZeroMQNode>(node1.getImpl());
		boost::shared_ptr<ZeroMQNode> nodeImpl2 = boost::static_pointer_cast<ZeroMQNode>(node2.getImpl());

//		assert(nodeImpl1->_nodes.find(nodeImpl2->getUUID()) != nodeImpl1->_nodes.end());
//		assert(nodeImpl2->_nodes.find(nodeImpl1->getUUID()) != nodeImpl2->_nodes.end());

		Publisher pub("foo");
		node1.addPublisher(pub);

		for (int j = 0; j < 2; j++) {
			Subscriber sub1("foo", NULL);
			Subscriber sub2("foo", NULL);
			Subscriber sub3("foo", NULL);

			int subs = 0;
			(void)subs;

			subs = pub.waitForSubscribers(0);
			assert(subs == 0);

			Thread::sleepMs(300);
			// node2 is aware of node1 publisher
//			assert(nodeImpl2->_nodes[nodeImpl1->getUUID()].getImpl()->_pubs.find(pub.getUUID()) != nodeImpl2->_nodes[nodeImpl1->getUUID()].getImpl()->_pubs.end());

			node2.addSubscriber(sub1);
			std::cout << "Waiting for 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(1);
			assert(subs == 1);

			node2.addSubscriber(sub2);
			std::cout << "Waiting for 2nd subscriber" << std::endl;
			subs = pub.waitForSubscribers(2);
			assert(subs == 2);

			node2.addSubscriber(sub3);
			std::cout << "Waiting for 3rd subscriber" << std::endl;
			subs = pub.waitForSubscribers(3);
			assert(subs == 3);

			node2.removeSubscriber(sub1);
			Thread::sleepMs(300);
			std::cout << "Removed 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(0);
			assert(subs == 2);

			node2.removeSubscriber(sub2);
			std::cout << "Removed 2nd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 1);

			node2.removeSubscriber(sub3);
			std::cout << "Removed 3rd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 0);

//			node1.removePublisher(pub);

			std::cout << "--- Successfully connected subscribers to publishers" << std::endl;

		}
	}
	return true;
}

int main(int argc, char** argv, char** envp) {
//	setenv("UMUNDO_LOGLEVEL", "4", 1);
	if (!testNodeDiscovery())
		return EXIT_FAILURE;
	if (!testNodeRemoval())
		return EXIT_FAILURE;
	if (!testPubSubConnections())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;


}
