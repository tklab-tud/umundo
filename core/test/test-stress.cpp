#include "umundo/core.h"

using namespace umundo;

bool testDiscoveryStress() {

	Node n;
	Publisher p("foo");
	n.addPublisher(p);

	assert(NodeImpl::instances == 1);

	for(int i = 1; i < 10; i++) {
		std::cout << "--- " << i << std::endl;
		Node n2;
		assert(NodeImpl::instances == 2);

		for (int j = 0; j < i; j++) {
			Subscriber s("foo", NULL);
			std::cout << "\tSub Implementations: " << SubscriberImpl::instances << std::endl;
			assert(SubscriberImpl::instances == j + 1);
			n2.addSubscriber(s);
			std::cout << "\tSubscribers: " << p.waitForSubscribers(0) << std::endl;
			assert(p.waitForSubscribers(j + 1) == j + 1);
			Thread::sleepMs(200);
		}
		assert(p.waitForSubscribers(i) == i);
		std::map<std::string, Subscriber> subs = n2.getSubscribers();
		std::map<std::string, Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			n2.removeSubscriber(subIter->second);
			subIter++;
		}
	}

	return true;
}

bool testSubscriberStress() {

	Node n;
	Node n2;
	assert(NodeImpl::instances == 2);
	Publisher p("foo");
	n.addPublisher(p);

	for(int i = 20; i < 40; i++) {
		std::cout << "--- " << i << std::endl;
		set<Subscriber> subs;
		for (int j = 0; j < i; j++) {
			Subscriber s("foo", NULL);
			n2.addSubscriber(s);
			subs.insert(s);
			std::cout << "\tSub Implementations: " << SubscriberImpl::instances << std::endl;
			assert(SubscriberImpl::instances == j + 1);
		}
		std::cout << "\tSub Implementations: " << SubscriberImpl::instances << std::endl;
		assert(SubscriberImpl::instances == i);
		assert(p.waitForSubscribers(i) == i);

		set<Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			n2.removeSubscriber(*subIter);
			subIter++;
		}
		subs.clear();
		assert(SubscriberImpl::instances == 0);

	}

	return true;
}

int main(int argc, char** argv) {
	if (!testDiscoveryStress())
		return EXIT_FAILURE;
	if (!testSubscriberStress())
		return EXIT_FAILURE;
	assert(NodeImpl::instances == 0);
	return EXIT_SUCCESS;
}