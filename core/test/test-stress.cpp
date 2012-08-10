#include "umundo/core.h"

using namespace umundo;

bool testDiscoveryStress() {

	Node* n = new Node();
	Publisher* p = new Publisher("foo");
	n->addPublisher(p);

	for(int i = 1; i < 10; i++) {
		std::cout << "--- " << i << std::endl;
		set<Node*> nodes;
		set<Subscriber*> subs;
		for (int j = 0; j < i; j++) {
			Node* n2 = new Node();
			Subscriber* s = new Subscriber("foo", NULL);
			n2->addSubscriber(s);
			nodes.insert(n2);
			subs.insert(s);
		}
		assert(p->waitForSubscribers(i) == i);

		set<Node*>::iterator nodeIter = nodes.begin();
		while(nodeIter != nodes.end()) {
			delete *nodeIter;
			nodeIter++;
		}

		set<Subscriber*>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			delete *subIter;
			subIter++;
		}

	}

	delete n;
	delete p;

	return true;
}

bool testSubscriberStress() {

	Node* n = new Node();
	Node* n2 = new Node();
	Publisher* p = new Publisher("foo");
	n->addPublisher(p);

	for(int i = 20; i < 40; i++) {
		std::cout << "--- " << i << std::endl;
		set<Subscriber*> subs;
		for (int j = 0; j < i; j++) {
			Subscriber* s = new Subscriber("foo", NULL);
			n2->addSubscriber(s);
			subs.insert(s);
		}
		assert(p->waitForSubscribers(i) == i);

		set<Subscriber*>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			n2->removeSubscriber(*subIter);
			delete *subIter;
			subIter++;
		}

	}

	delete n;
	delete n2;
	delete p;

	return true;
}

int main(int argc, char** argv) {
	if (!testDiscoveryStress())
		return EXIT_FAILURE;
	if (!testSubscriberStress())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}