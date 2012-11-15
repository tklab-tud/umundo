// I feel dirty!
#define protected public
#include "umundo/connection/zeromq/ZeroMQNode.h"

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
using namespace std;

char* domain = NULL;
bool verbose = false;

void printUsageAndExit() {
	printf("umundo-bridge version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-bridge [-d domain] [-v] protocol://endpoint:port\n");
	printf("\n");
	printf("Options\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-v                 : be more verbose\n");
	printf("\t-t                 : tunnel all messages\n");
	printf("\n");
	printf("Example\n");
	printf("\tumundo-bridge tcp://130.32.14.22:4242\n");	
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

	Node* node = NULL;

	if (domain) {
		node = new Node(domain);
	} else {
		node = new Node();
	}

	if (optind >= argc)
	 	printUsageAndExit();

	std::string hostname(argv[optind]);
	boost::shared_ptr<NodeStub> endPoint = boost::shared_ptr<NodeStub>(new NodeStub());

	size_t oldPos = 0;;
	size_t pos = 0;;
	if ((pos = hostname.find("://", pos)) != std::string::npos) {
		endPoint->setTransport(hostname.substr(0, pos));
		oldPos = pos + 3;
	} else {
		endPoint->setTransport("tcp");
		oldPos = pos;
	}

	if ((pos = hostname.find(":", pos + 1)) != std::string::npos) {
		endPoint->setIP(hostname.substr(oldPos, pos - oldPos));
		oldPos = pos + 1;
	} else {
		printUsageAndExit();
	}
	
	if (pos + 1 < hostname.length()) {
		string port(hostname.substr(oldPos));
		endPoint->setPort(strTo<uint16_t>(port));
	} else {
		printUsageAndExit();		
	}

	boost::static_pointer_cast<umundo::ZeroMQNode>(node->_impl)->added(endPoint);

	while (true) {
		Thread::sleepMs(500);
	}

}

