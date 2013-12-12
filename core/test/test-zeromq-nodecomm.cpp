#include "umundo/common/Factory.h"
#include "umundo/common/Message.h"
#include "umundo/connection/Node.h"

using namespace umundo;

bool testNodeConnections() {
	int iterations = 10;
	
	// simple connection from node to node
	while (iterations--) {
		Node* node1 = new Node();
		Node* node2 = new Node();
		
		// connect both nodes and make sure they know each other
		node1->added(*node2);
		node2->added(*node1);
		
		usleep(100000);
		std::map<std::string, NodeStub> peers1 = node1->connectedTo();
		std::map<std::string, NodeStub> peers2 = node2->connectedTo();
		
		assert(peers1.size() == 1);
		assert(peers1.find(node2->getUUID()) != peers1.end());
		assert(peers1.begin()->second.getUUID() == node2->getUUID());
		assert(peers1.begin()->second.getAddress() == node2->getAddress());

		assert(peers2.size() == 1);
		assert(peers2.find(node1->getUUID()) != peers2.end());
		assert(peers2.begin()->second.getUUID() == node1->getUUID());
		assert(peers2.begin()->second.getAddress() == node1->getAddress());

		delete node1;
		usleep(100000);

		peers2 = node2->connectedTo();
		assert(peers2.size() == 0);
		
		delete node2;
	}
	
	// half connection with one node connected to the other one
	iterations = 10;
	while (iterations--) {
		Node* node1 = new Node();
		Node* node2 = new Node();
		
		// connect both nodes and make sure they know each other
		node1->added(*node2);
		
		usleep(100000);
		std::map<std::string, NodeStub> peers1 = node1->connectedTo();
		std::map<std::string, NodeStub> peers2 = node2->connectedFrom();
		
		assert(peers1.size() == 1);
		assert(peers1.find(node2->getUUID()) != peers1.end());
		assert(peers1.begin()->second.getUUID() == node2->getUUID());
		assert(peers1.begin()->second.getAddress() == node2->getAddress());
		
		assert(peers2.size() == 1);
		assert(peers2.find(node1->getUUID()) != peers2.end());
		assert(peers2.begin()->second.getUUID() == node1->getUUID());
		

		// add other node just now
		node2->added(*node1);
		usleep(100000);

		assert(peers1.size() == 1);
		assert(peers1.find(node2->getUUID()) != peers1.end());
		assert(peers1.begin()->second.getUUID() == node2->getUUID());
		assert(peers1.begin()->second.getAddress() == node2->getAddress());
		
		assert(peers2.size() == 1);
		assert(peers2.find(node1->getUUID()) != peers2.end());
		assert(peers2.begin()->second.getUUID() == node1->getUUID());
		assert(peers2.begin()->second.getAddress() == node1->getAddress());

		delete node1;
		delete node2;
	}

	return true;
}

bool testGeneralStuff() {
	Node* node1 = new Node();
	int iterations = 10;
	while (iterations--) {
		
		Publisher pub1("channel1");
		Publisher pub2("channel2");
		Subscriber sub1("channel1");
		Node* node2 = new Node();
		
		assert(node2->getUUID().length() == 36);
		assert(node2->getTransport() == "tcp");
		assert(node2->getIP() == "127.0.0.1");
//		assert(node2->getPort() == 20001);
		
		node2->addPublisher(pub1);
		usleep(10000);

		Publisher pub = node2->getPublisher(pub1.getUUID());
		assert(pub == pub1);
		
		node1->added(*node2);
		usleep(100000);

		// get peers of node1 and assert that node2 is known qualified
		std::map<std::string, NodeStub> peers;
		std::map<std::string, PublisherStub> pubs;
		peers = node1->connectedTo();
		assert(peers.size() == 1);
		assert(peers.begin()->second.getUUID() == node2->getUUID());
		assert(peers.begin()->second.getTransport() == node2->getTransport());
		assert(peers.begin()->second.getIP() == node2->getIP());
		assert(peers.begin()->second.getPort() == node2->getPort());
		
		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 1);
		assert(pubs.begin()->second.getUUID() == pub1.getUUID());
		
		// node2 knows node1 as an unqualified peer
		peers = node2->connectedFrom();
		assert(peers.size() == 1);
		assert(peers.begin()->second.getUUID() == node1->getUUID());
		assert(peers.begin()->second.getIP().length() == 0);
		assert(peers.begin()->second.getPort() == 0);
		assert(peers.begin()->second.getTransport().length() == 0);

		node1->addSubscriber(sub1);
		usleep(10000);

		// sub1 was connected to pub1
		std::map<std::string, PublisherStub> otherPubs = sub1.getPublishers();
		assert(otherPubs.size() == 1);
		assert(otherPubs.find(pub1.getUUID()) != otherPubs.end());
		
		// assert that we can send and receive
		pub1.send("asdf", 4);
		usleep(10000);
		assert(sub1.hasNextMsg());
		Message* msg = sub1.getNextMsg();
		assert(msg->size() == 4);
		assert(strcmp(msg->data(), "asdf") == 0);
		delete msg;
		
		node1->removeSubscriber(sub1);
		usleep(10000);
		otherPubs = sub1.getPublishers();
		assert(otherPubs.size() == 0);

		node2->addPublisher(pub2);
		usleep(10000);

		peers = node1->connectedTo();
		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 2);
		assert(pubs.find(pub1.getUUID()) != pubs.end());
		assert(pubs.find(pub2.getUUID()) != pubs.end());
		
		node2->removePublisher(pub1);
		usleep(10000);

		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 1);
		assert(pubs.begin()->second.getUUID() == pub2.getUUID());

		// now connect the other way around
		node2->added(*node1);
		usleep(10000);
		
		// both nodes know each other fully qualified now
		peers = node1->connectedTo();
		assert(peers.size() == 1);
		assert(peers.begin()->second.getUUID() == node2->getUUID());
		assert(peers.begin()->second.getTransport() == node2->getTransport());
		assert(peers.begin()->second.getIP() == node2->getIP());
		assert(peers.begin()->second.getPort() == node2->getPort());

		peers = node2->connectedTo();
		assert(peers.size() == 1);
		assert(peers.begin()->second.getUUID() == node1->getUUID());
		assert(peers.begin()->second.getTransport() == node1->getTransport());
		assert(peers.begin()->second.getIP() == node1->getIP());
		assert(peers.begin()->second.getPort() == node1->getPort());

		// activate pub1 on node1
		node2->removePublisher(pub2);
		node1->addPublisher(pub1);
		usleep(100000);

		// check that each node has a current view of the others publishers
		peers = node1->connectedTo();
		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 0);

		peers = node2->connectedTo();
		assert(peers.size() == 1);
		assert(peers.begin()->second.getUUID() == node1->getUUID());
		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 1);
		assert(pubs.begin()->second.getUUID() == pub1.getUUID());
		assert(pubs.begin()->second.getTransport() == pub1.getTransport());

		peers = node1->connectedTo();
		pubs = peers.begin()->second.getPublishers();
		assert(pubs.size() == 0);
		usleep(10000);

		node1->removePublisher(pub1);

		delete node2;
	}
	delete node1;
	return true;
}

int main(int argc, char** argv) {
	setenv("UMUNDO_LOGLEVEL", "4", 1);
	if (!testNodeConnections())
		return EXIT_FAILURE;
	if (!testGeneralStuff())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;

}