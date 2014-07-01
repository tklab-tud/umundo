// I feel dirty!
#define protected public
#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/discovery/MDNSDiscovery.h"

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
#include <string.h>
#include <iostream>
#include <fstream>

#ifdef WIN32
#include "XGetopt.h"
#endif

using namespace umundo;

char* domain = NULL;
bool verbose = false;

void printUsageAndExit() {
	printf("umundo-bridge version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-bridge [-d domain] [-v] protocol://localIP:port protocol://remoteIP:port\n");
	printf("\n");
	printf("Options\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-v                 : be more verbose\n");
	printf("\t-t                 : tunnel all messages\n");
	printf("\n");
	printf("Example\n");
	
	printf("\tumundo-bridge tcp://130.32.14.22:4242 tcp://53.2.23.72:4242\n");
	exit(1);
}

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "vd:")) != -1) {
		switch(option) {
		case 'd':
			domain = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (optind >= argc - 1)
		printUsageAndExit();

	std::string localAddress = argv[optind];
	std::string remoteAddress = argv[optind + 1];
	
	MDNSDiscoveryOptions mdnsOpts;
	if (domain)
		mdnsOpts.setDomain(domain);
	Discovery disc(Discovery::MDNS, &mdnsOpts);
	
//	Node innerNode;
//	disc.add(innerNode);
	
	EndPoint endPoint(remoteAddress);
	if (!endPoint)
		printUsageAndExit();

	Node outerNode(localAddress);
	outerNode.added(endPoint);

	Publisher pub("foo");
	outerNode.addPublisher(pub);
	
	while (true) {
		Thread::sleepMs(500);
	}

}

