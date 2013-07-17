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
#include <cstdio>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#ifdef WIN32
#include "XGetopt.h"
#endif

#ifdef WIN32
std::string pathSeperator = "\\";
#else
std::string pathSeperator = "/";
#endif

using namespace umundo;
using namespace std;

std::string channel;
std::string domain;
std::string file;
FILE *fp;
bool verbose = false;
uint64_t startedAt = 0;

void printUsageAndExit() {
	printf("umundo-capture version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-capture -c channel [-v] [-d domain] -f file\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c <channel>       : use channel\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-v                 : be more verbose\n");
	printf("\tfile               : filename to write captured data to\n");
	exit(1);
}

class LoggingReceiver : public Receiver {
	void receive(Message* msg) {
		
		uint64_t timeDiff = Thread::getTimeStampMs() - startedAt;
		map<string, string>::const_iterator metaIter;

		// get size of message
		uint64_t msgSize = 0;
		
		msgSize += sizeof(timeDiff);
		metaIter = msg->getMeta().begin();
		while(metaIter != msg->getMeta().end()) {
			msgSize += metaIter->first.size() + 1;
			msgSize += metaIter->second.size() + 1;			
			metaIter++;
		}
		msgSize += 2;
		msgSize += sizeof(msg->size());
		msgSize += msg->size();
		
		fwrite(&msgSize, sizeof(msgSize), 1, fp); // size of overall message 
		fwrite(&timeDiff, sizeof(timeDiff), 1, fp); // time difference

		metaIter = msg->getMeta().begin();
		while(metaIter != msg->getMeta().end()) {
			fwrite(metaIter->first.c_str(), metaIter->first.size() + 1, 1, fp);
			fwrite(metaIter->second.c_str(), metaIter->second.size() + 1, 1, fp);
			metaIter++;
		}
		fwrite("\0\0", 2, 1, fp); // meta / data seperator
		size_t size = msg->size();
		fwrite(&size, sizeof(msg->size()), 1, fp); // length prefix
		fwrite(msg->data(), msg->size(), 1, fp); // data
		
		std::cout << msgSize << "B" << ".";
		std::flush(std::cout);
		
	}
};

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "vd:f:c:w:p:")) != -1) {
		switch(option) {
		case 'c':
			channel = optarg;
			break;
		case 'd':
			domain = optarg;
			break;
		case 'f':
			file = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (!channel.length() > 0)
		printUsageAndExit();

	if (!file.length() > 0)
		printUsageAndExit();

	Node node(domain);
	Subscriber sub(channel);

	fp = fopen(file.c_str(), "w");
	if (fp == NULL) {
		printf("Failed to open file %s: %s\n", file.c_str(), strerror(errno));
		return EXIT_FAILURE;
	}

	sub.setReceiver(new LoggingReceiver());
	node.addSubscriber(sub);

	startedAt = Thread::getTimeStampMs();
	
	std::cout << "Capturing packets from " << channel << std::endl;
	std::cout << "Press return to exit" << std::endl;
	
	std::string line;
	std::getline(std::cin, line);

	node.removeSubscriber(sub);
	sub.setReceiver(NULL); // make sure receiver gets no more messages

	fclose(fp);

}

