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
int minSubs = 0;

void printUsageAndExit() {
	printf("umundo-replay version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-replay -c channel [-w N] [-v] [-d domain] -f file\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c <channel>       : use channel\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-w <number>        : wait for given number of subscribers before publishing\n");
	printf("\t-v                 : be more verbose\n");
	printf("\tfile               : filename to read captured data from\n");
	exit(1);
}

class LoggingReceiver : public Receiver {
	void receive(Message* msg) {
		uint64_t timeDiff = Thread::getTimeStampMs() - startedAt;
		fwrite(&timeDiff, sizeof(timeDiff), 1, fp); // time difference
		
		map<string, string>::const_iterator metaIter = msg->getMeta().begin();
		while(metaIter != msg->getMeta().end()) {
			fwrite(&metaIter->first, metaIter->first.size() + 1, 1, fp);
			fwrite(&metaIter->second, metaIter->second.size() + 1, 1, fp);
			metaIter++;
		}
		fwrite("\0\0", 2, 1, fp); // meta / data seperator
		size_t size = msg->size();
		fwrite(&size, sizeof(msg->size()), 1, fp); // length prefix
		fwrite(msg->data(), msg->size(), 1, fp); // data
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
		case 'w':
			minSubs = atoi((const char*)optarg);
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
	Publisher pub(channel);

	fp = fopen(file.c_str(), "r");
	if (fp == NULL) {
		printf("Failed to open file %s: %s\n", file.c_str(), strerror(errno));
		return EXIT_FAILURE;
	}

	node.addPublisher(pub);
	pub.waitForSubscribers(minSubs);
	
	startedAt = Thread::getTimeStampMs();
	
	std::cout << "Writing packets to channel " << channel << std::endl;

	while(true) {
		uint64_t msgSize;
		size_t read = 0;
		read = fread(&msgSize, sizeof(uint64_t), 1, fp);
		if (!read)
			break;
		
		char* buffer = new char[msgSize];
		char* readPos = buffer;
		read = fread(buffer, msgSize, 1, fp);
		if (!read)
			break;

		uint64_t timeDiff = *(uint64_t*)(readPos);
		readPos += sizeof(uint64_t);

		Message* msg = new Message();
		
		while (*readPos) {
			std::string key(readPos);
			readPos += key.length() + 1;
			std::string value(readPos);
			readPos += value.length() + 1;
			msg->putMeta(key, value);
		}
		
		readPos++;
		uint64_t dataSize = *(uint64_t*)(readPos);
		readPos += sizeof(uint64_t);
		msg->setData(readPos, dataSize);
		
		uint64_t now = Thread::getTimeStampMs();
		if (now < startedAt + timeDiff) {
			Thread::sleepMs((startedAt + timeDiff) - now);
		}
		
		pub.send(msg);
		
		delete(msg);
		free(buffer);
	}
	
	node.removePublisher(pub);

	fclose(fp);

}

