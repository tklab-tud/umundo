#include "umundo/core.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024*1024

using namespace umundo;

bool testStackInstantiation() {
	Node node1;
	Node node2;
	Publisher pub("foo");
	Subscriber sub("foo");
	node1.addPublisher(pub);
	node2.addSubscriber(sub);
	Thread::sleepMs(100);
	return true;
}

bool testSTLContainers() {
	{
		// single item in container
		Node n;
		std::set<Node> nodeSet;
		nodeSet.insert(n);
		assert(nodeSet.find(n) != nodeSet.end());
		nodeSet.erase(n);
		assert(nodeSet.size() == 0);
		// copy of item ought to be identical
		Node n2 = n;
		assert(n == n2);
		assert(!(n != n2));
		nodeSet.insert(n2);
		assert(nodeSet.find(n) != nodeSet.end());
		assert(nodeSet.find(n2) != nodeSet.end());
		nodeSet.erase(n);
		assert(nodeSet.size() == 0);
	}
	{
		Publisher p;
		std::set<Publisher> pubSet;
		pubSet.insert(p);
		assert(pubSet.find(p) != pubSet.end());
		pubSet.erase(p);
		assert(pubSet.size() == 0);
		Publisher p2 = p;
		assert(p == p2);
		assert(!(p != p2));
		pubSet.insert(p2);
		assert(pubSet.find(p) != pubSet.end());
		assert(pubSet.find(p2) != pubSet.end());
		pubSet.erase(p);
		assert(pubSet.size() == 0);
	}
	{
		Subscriber s;
		std::set<Subscriber> subSet;
		subSet.insert(s);
		assert(subSet.find(s) != subSet.end());
		subSet.erase(s);
		assert(subSet.size() == 0);
		Subscriber s2 = s;
		assert(s == s2);
		assert(!(s != s2));
		subSet.insert(s2);
		assert(subSet.find(s) != subSet.end());
		assert(subSet.find(s2) != subSet.end());
		subSet.erase(s);
		assert(subSet.size() == 0);

	}

	{
		EndPoint ep1("tcp://127.0.0.1:8080");
		EndPoint ep2("tcp://127.0.0.1:8080");
		EndPoint ep3("tcp://127.0.0.1:8090");
		EndPoint ep4("tcp://127.0.0.1:8080");
		ep4.getImpl()->setUUID("asdfadsf");

		assert(ep1 == ep2);
		assert(ep1 != ep3);
		assert(ep1 != ep4);

		std::set<EndPoint> endPoints;
		endPoints.insert(ep1);
		assert(endPoints.size() == 1);

		endPoints.insert(ep2);
		assert(endPoints.size() == 1);
		assert(endPoints.find(ep1) != endPoints.end());
		assert(endPoints.find(ep2) != endPoints.end());

		endPoints.insert(ep3);
		assert(endPoints.size() == 2);
		assert(endPoints.find(ep3) != endPoints.end());

		assert(endPoints.find(ep4) == endPoints.end());
		endPoints.insert(ep4);
		assert(endPoints.size() == 3);
		assert(endPoints.find(ep4) != endPoints.end());

	}
	return true;
}

int main(int argc, char** argv, char** envp) {
	if(!testStackInstantiation())
		return EXIT_FAILURE;
	if(!testSTLContainers())
		return EXIT_FAILURE;

	return EXIT_SUCCESS;

}
