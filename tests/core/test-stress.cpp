#include "umundo/core.h"

using namespace umundo;

bool testDiscoveryStress() {

	Node n;
	Publisher p("foo");
	n.addPublisher(p);

	for(int i = 40; i > 0; i--) {
		Node n2;

		n2.added(n);
		n.added(n2);

		for (int j = 0; j < i; j++) {
			Subscriber s("foo");
			n2.addSubscriber(s);
			int subs = p.waitForSubscribers(j);
			std::cout << "\tSubscribers: " << subs << std::endl;
//			assert(subs == j);
		}

		assert(p.waitForSubscribers(i) == i);

		std::map<std::string, Subscriber> subs = n2.getSubscribers();
		std::map<std::string, Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			n2.removeSubscriber(subIter->second);
			subIter++;
		}

		n2.removed(n);
		n.removed(n2);

	}

	return true;
}

bool testSubscriberStress() {

	Node n;
	Node n2;
	assert(NodeImpl::instances == 2);
	Publisher p("foo");
	n.addPublisher(p);

	for(int i = 10; i < 20; i++) {
		std::cout << "--- " << i << std::endl;
		std::set<Subscriber> subs;
		for (int j = 0; j < i; j++) {
			Subscriber s("foo");
			n2.addSubscriber(s);
			subs.insert(s);
			std::cout << "\tSub Implementations: " << SubscriberImpl::instances << std::endl;
		}
		std::cout << "\tSub Implementations: " << SubscriberImpl::instances << std::endl;
		assert(p.waitForSubscribers(i) == i);

		std::set<Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			n2.removeSubscriber(*subIter);
			subIter++;
		}
		subs.clear();
		assert(SubscriberImpl::instances == 0);

	}

	return true;
}

class StressReceiver : public Receiver {
	void receive(Message* msg) {
		std::cout << "i";
	}
};

void testContinuousStress() {
	char* payload = (char*)malloc(4096);
	for (int i = 0; i < 4096; i++) {
		payload[i] = (char)i;
	}

	Discovery disc(Discovery::MDNS);
	Node node1;
	StressReceiver rcv;
	disc.add(node1);

	while(true) {
		Publisher pub1("stress");
		Subscriber sub1("stress");
		sub1.setReceiver(&rcv);
		
		node1.addPublisher(pub1);
		node1.addSubscriber(sub1);


		int i = 500;
		while(i-- > 0) {
			std::cout << "o";
			Message* msg = new Message(payload, 4096);
			pub1.send(msg);
			delete msg;
			Thread::sleepMs(10);
		}

		node1.removePublisher(pub1);
		node1.removeSubscriber(sub1);

	}
	disc.remove(node1);
}

int main(int argc, char** argv) {
#if 1

//	testContinuousStress();
	if (!testDiscoveryStress())
		return EXIT_FAILURE;
//	if (!testSubscriberStress())
//		return EXIT_FAILURE;
	assert(NodeImpl::instances == 0);
	return EXIT_SUCCESS;
#endif
}