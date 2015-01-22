/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 */

#include "umundo/config.h"
#include "umundo/core.h"
#include <iostream>
#include <string.h>
#ifdef WIN32
#include "XGetopt.h"
#endif

using namespace umundo;

char* domain = NULL;

class TestReceiver : public Receiver {
public:
	TestReceiver() {};
	void receive(Message* msg) {
		std::cout << "i" << std::flush;
	}
};

void printUsageAndExit() {
	printf("Usage\n");
	printf("\tumundo-pingpong [-d domain]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\n");
	printf("Example\n");

	printf("\tumundo-bridge -d local\n");
	exit(1);
}

int main(int argc, char** argv) {
	printf("umundo-pingpong version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	int option;
	while ((option = getopt(argc, argv, "d:")) != -1) {
		switch(option) {
		case 'd':
			domain = optarg;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	TestReceiver* testRecv = new TestReceiver();
	Publisher pubFoo("pingpong");
	Subscriber subFoo("pingpong", testRecv);

	MDNSDiscoveryOptions mdnsOpts;
	if (domain)
		mdnsOpts.setDomain(domain);
	Discovery disc(Discovery::MDNS, &mdnsOpts);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);
	node.addSubscriber(subFoo);

	while(1) {
		Thread::sleepMs(1000);
		Message* msg = new Message();
		msg->setData("ping", 4);
		std::cout << "o" << std::flush;
		pubFoo.send(msg);
		delete(msg);
	}
}
