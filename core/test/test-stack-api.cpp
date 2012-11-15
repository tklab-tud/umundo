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
	Thread::sleepMs(10000);
	return true;
}

int main(int argc, char** argv, char** envp) {
	if(!testStackInstantiation())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
