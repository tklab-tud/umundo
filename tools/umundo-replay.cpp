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
#include "umundo.h"
#include "umundo/discovery/MDNSDiscovery.h"
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

std::string channel;
std::string domain;
std::string file;
FILE *fp;
bool verbose = false;
bool interactive = false;
bool trim = false;
bool loop = false;
uint64_t startedAt = 0;
int minSubs = 0;

void printUsageAndExit() {
	printf("umundo-replay version " UMUNDO_VERSION " (" UMUNDO_PLATFORM_ID " " CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-replay -c channel [-w N] [-vit] [-d domain] -f file\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c <channel>       : use channel\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-w <number>        : wait for given number of subscribers before publishing\n");
	printf("\t-v                 : be more verbose\n");
	printf("\t-l                 : play input file in loop\n");
	printf("\t-i                 : interactive mode (ignore timestamp, send after after return pressed)\n");
	printf("\t-t                 : trim initial delay, start with first message immediately\n");
	printf("\tfile               : filename to read captured data from\n");
	exit(1);
}

int main(int argc, char** argv) {
	int option;
	while ((option = getopt(argc, argv, "viltd:f:c:w:p:")) != -1) {
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
		case 'i':
			interactive = true;
			break;
		case 'l':
			loop = true;
			break;
		case 't':
			trim = true;
			break;
		case 'w':
			minSubs = atoi((const char*)optarg);
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (channel.length() == 0)
		printUsageAndExit();

	if (file.length() == 0)
		printUsageAndExit();

	DiscoveryConfigMDNS mdnsOpts;
	if (domain.length() > 0)
		mdnsOpts.setDomain(domain);
	Discovery disc(&mdnsOpts);

	Node node;
	disc.add(node);

	Publisher pub(channel);

	fp = fopen(file.c_str(), "r");
	if (fp == NULL) {
		printf("Failed to open file %s: %s\n", file.c_str(), strerror(errno));
		return EXIT_FAILURE;
	}

	node.addPublisher(pub);

	if (minSubs) {
		if (verbose)
			std::cout << "Waiting for " << minSubs << " subscribers" << std::endl;
		pub.waitForSubscribers(minSubs);
	}

	if (verbose)
		std::cout << "Writing packets to channel '" << channel << "'" << std::endl;

	do {
		startedAt = Thread::getTimeStampMs();
		fseek(fp, 0, 0);

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

			if (trim) {
				startedAt -= timeDiff;
				trim = false;
			}

			Message* msg = new Message();

			while (*readPos) {
				std::string key(readPos);
				readPos += key.length() + 1;
				std::string value(readPos);
				readPos += value.length() + 1;
				msg->putMeta(key, value);
			}

			readPos += 2;
			uint64_t dataSize = readPos - buffer;
//			readPos += sizeof(uint64_t);
			msg->setData(readPos, dataSize);

			uint64_t now = Thread::getTimeStampMs();
			if (interactive) {
				std::cout << "Press return to send next message" << std::endl;
				std::string line;
				std::getline(std::cin, line);
			} else if (now < startedAt + timeDiff) {
				if (verbose)
					std::cout << "Waiting " << (startedAt + timeDiff) - now << "ms" << std::endl;
				Thread::sleepMs((startedAt + timeDiff) - now);
			}

			pub.send(msg);
			if (verbose)
				std::cout << "Published " << msgSize << " bytes" << std::endl;

			delete(msg);
			free(buffer);
		}
	} while(loop);

// triggers an assert otherwise?
	Thread::sleepMs(200);
	node.removePublisher(pub);

	fclose(fp);

}

