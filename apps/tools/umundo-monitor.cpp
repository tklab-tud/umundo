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
#include "umundo/s11n.h"
#include "umundo/s11n/protobuf/PBSerializer.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#ifdef WIN32
#include "XGetopt.h"
#endif

#ifdef DISC_BONJOUR
#include "umundo/discovery/bonjour/BonjourNodeDiscovery.h"
#define DISC_IMPL BonjourNodeDiscovery
#endif
#ifdef DISC_AVAHI
#include "umundo/discovery/avahi/AvahiNodeDiscovery.h"
#define DISC_IMPL AvahiNodeDiscovery
#endif
#if !(defined DISC_AVAHI || defined DISC_BONJOUR)
#error "No discovery implementation choosen"
#endif

#ifdef S11N_PROTOBUF
#include "umundo/s11n/TypedSubscriber.h"
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/descriptor.h>
#else
#error "No serialization implementation choosen"
#endif

#ifdef NET_ZEROMQ
#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"
#define NET_NODE_IMPL ZeroMQNode
#define NET_PUB_IMPL ZeroMQPublisher
#define NET_SUB_IMPL ZeroMQSubscriber
#endif
#if !(defined NET_ZEROMQ)
#error "No discovery implementation choosen"
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
std::string protoPath;
bool interactive = false;
bool verbose = false;
int minSubs = 0;

class DiscoveryMonitor : public DISC_IMPL {};
class NodeMonitor : public NET_NODE_IMPL {};
class PublisherMonitor : public NET_PUB_IMPL {};
class SubscriberMonitor : public NET_SUB_IMPL {};

void printUsageAndExit() {
	printf("umundo-monitor version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-monitor -c channel [-iv] [-d domain] [-f file] [-p dir]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-c <channel>       : use channel\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-f <file>          : publish contents of file\n");
	printf("\t-w <number>        : wait for given number of subscribers before publishing\n");
	printf("\t-i                 : interactive mode (simple chat)\n");
	printf("\t-v                 : be more verbose\n");
	printf("\t-p <dir>           : path with .pb.desc files for runtime reflection of protobuf messages\n");
	exit(1);
}

class PlainDumpingReceiver : public TypedReceiver {
	void receive(void* object, Message* msg) {
		map<string, string>::const_iterator metaIter = msg->getMeta().begin();
		while(metaIter != msg->getMeta().end()) {
			std::cout << metaIter->first << ": " << metaIter->second << std::endl;
			metaIter++;
		}
		std::string seperator;
		for (int i = 0; i < msg->size(); i++) {
			std::cout << seperator << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(static_cast<unsigned char>(msg->data()[i]));
			seperator = ":";
		}
		std::cout << std::endl;
	}
};

class ProtoBufDumpingReceiver : public TypedReceiver {
public:
	ProtoBufDumpingReceiver() {
	}
	void receive(void* object, Message* msg) {
		// dump meta fields in any case
		std::map<std::string, std::string>::const_iterator metaIter = msg->getMeta().begin();
		std::cout << "Meta Fields:" << std::endl;
		while(metaIter != msg->getMeta().end()) {
			std::cout << "  " << metaIter->first << ": " << metaIter->second << std::endl;
			metaIter++;
		}

		if (object != NULL) {
			google::protobuf::Message* pbObj = (google::protobuf::Message*)object;
			pbObj->PrintDebugString();
		}
	}
};

int main(int argc, char** argv) {
	// Factory::registerPrototype("discovery", new DiscoveryMonitor(), NULL);
	// Factory::registerPrototype("node", new NodeMonitor(), NULL);
	// Factory::registerPrototype("publisher", new PublisherMonitor(), NULL);
	// Factory::registerPrototype("subscriber", new SubscriberMonitor(), NULL);

	int option;
	while ((option = getopt(argc, argv, "ivd:f:c:w:p:")) != -1) {
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
		case 'w':
			minSubs = atoi((const char*)optarg);
			break;
		case 'i':
			interactive = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'p':
			protoPath = optarg;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (!channel.length() > 0)
		printUsageAndExit();

	Node node(domain);
	TypedPublisher pub(channel);
	TypedSubscriber sub(channel);

	/**
	 * Send file content
	 */
	if (file.length() > 0) {
		node.addPublisher(pub);
		pub.waitForSubscribers(minSubs);

		FILE *fp;
		fp = fopen(file.c_str(), "r");
		if (fp == NULL) {
			printf("Failed to open file %s: %s\n", file.c_str(), strerror(errno));
			return EXIT_FAILURE;
		}

		int read = 0;
		int lastread = 0;
		char* readBuffer = (char*) malloc(1000);
		while(true) {
			lastread = fread(readBuffer, 1, 1000, fp);

			if(ferror(fp)) {
				printf("Failed to read from file %s: %s", file.c_str(), strerror(errno));
				return EXIT_FAILURE;
			}

			if (lastread <= 0)
				break;

			pub.send(readBuffer, lastread);
			read += lastread;

			if (feof(fp))
				break;
		}
		fclose(fp);
		printf("Send %d bytes on channel \"%s\"\n", read, channel.c_str());
		if (!interactive)
			exit(0);
	}

	if (protoPath.length() > 0) {
		PBSerializer::addProto(protoPath);
		sub.setReceiver(new ProtoBufDumpingReceiver());
	} else {
		sub.setReceiver(new PlainDumpingReceiver());
	}
	node.addSubscriber(sub);

	if (interactive) {
		/**
		 * Enter interactive mode
		 */
		node.addPublisher(pub);

		//pub->waitForSubscribers(minSubs);
		string line;

		Message* msg = new Message();
		while(std::cin) {
			getline(std::cin, line);
			if (line.substr(0,4).compare("meta") == 0) {
				size_t keyStart = line.find_first_not_of(" ", 4);
				size_t keyEnd = line.find_first_of(" =", keyStart);
				size_t valueStart = line.find_first_not_of(" =", keyEnd);
				size_t valueEnd = line.length();

				string key = line.substr(keyStart, keyEnd - keyStart);
				string value = line.substr(valueStart, valueEnd - valueStart);
				cout << key << " = " << value << endl;
				msg->putMeta(key, value);
				continue;
			} else {
				line.append("\n");
				msg->setData(line.c_str(), line.length());
			}
			pub.send(msg);
			delete msg;
			msg = new Message();
		};
		delete msg;
	} else {
		/**
		 * Non-interactive, just let the subscriber print channel messages
		 */
		while (true)
			Thread::sleepMs(500);
	}

}

