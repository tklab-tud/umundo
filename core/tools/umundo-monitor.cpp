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
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>

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

char* channel = NULL;
char* domain = NULL;
char* file = NULL;
char* protoPath = NULL;
bool interactive = false;
bool verbose = false;
int minSubs = 0;

class DiscoveryMonitor : public DISC_IMPL {};
class NodeMonitor : public NET_NODE_IMPL {};
class PublisherMonitor : public NET_PUB_IMPL {};
class SubscriberMonitor : public NET_SUB_IMPL {};

void printUsageAndExit() {
	printf("umundo-monitor version 0.0.2\n");
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
	printf("\n");
	printf("\t-p <dir>           : path with .pb.desc files for runtime reflection of protobuf messages\n");
	exit(1);
}

class PlainDumpingReceiver : public Receiver {
	void receive(Message* msg) {
		map<string, string>::const_iterator metaIter = msg->getMeta().begin();
		while(metaIter != msg->getMeta().end()) {
			std::cout << metaIter->first << ": " << metaIter->second << std::endl;			
			metaIter++;
		}
		std::cout << string(msg->data(), msg->size()) << std::flush;
	}
};

class ProtoBufDumpingReceiver : public Receiver {
public:
	ProtoBufDumpingReceiver(const string pbPath) {
		_pbPath = pbPath;
	}
	void receive(Message* msg) {
		// dump meta fields in any case
		std::map<std::string, std::string>::const_iterator metaIter = msg->getMeta().begin();
		std::cout << "Meta Fields:" << std::endl;
		while(metaIter != msg->getMeta().end()) {
			std::cout << "  " << metaIter->first << ": " << metaIter->second << std::endl;
			metaIter++;
		}

		string type = msg->getMeta("type");
		if (type.length() > 0) {
			// the following code is adapted from http://www.mail-archive.com/protobuf@googlegroups.com/msg04058.html
			if (descriptor_pool.FindMessageTypeByName(type) == NULL) {
				// no descriptor is known yet, try to read from file
				string filename = _pbPath;
				filename.append(pathSeperator);
				filename.append(type); // This will fail if we only receive contained types
				filename.append(".pb.desc");
				std::ifstream desc_file(filename.c_str() ,std::ios::in|std::ios::binary);

				if (desc_file.good()) {
					google::protobuf::FileDescriptorSet f;
					f.ParseFromIstream(&desc_file);
					// f.PrintDebugString();
					// std::cout << std::endl;

					for (int i = 0; i < f.file_size(); ++i) {
						descriptor_pool.BuildFile(f.file(i));
					}
				}
			}

			if (descriptor_pool.FindMessageTypeByName(type) != NULL) {
				std::cout << "Protobuf Object:" << std::endl;
				// Using the descriptor to get a Message.
				const google::protobuf::Descriptor* descriptor = descriptor_pool.FindMessageTypeByName(type);
				google::protobuf::DynamicMessageFactory factory;
				google::protobuf::Message* protoMsg = factory.GetPrototype(descriptor)->New();

				// Parse protobuf from umundo message
				protoMsg->ParseFromString(std::string(msg->data(), msg->size()));

				// Print out the message
				protoMsg->PrintDebugString();
			} else {
				std::cout << "Raw " << type << ":" << string(msg->data(), msg->size()) << std::endl;
			}
		}
		std::cout << std::flush;
	}

	google::protobuf::DescriptorPool descriptor_pool;
	string _pbPath;
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
			interactive = true;
			break;
		case 'p':
			protoPath = optarg;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	if (!channel)
		printUsageAndExit();

	Node* node = NULL;
	Publisher* pub = NULL;
	Subscriber* sub = NULL;

	if (domain) {
		node = new Node(domain);
	} else {
		node = new Node();
	}

	/**
	 * Send file content
	 */
	if (file) {
		if (pub == NULL) {
			pub = new Publisher(channel);
			node->addPublisher(pub);
			pub->waitForSubscribers(minSubs);
		}

		FILE *fp;
		fp = fopen(file, "r");
		if (fp == NULL) {
			printf("Failed to open file %s: %s\n", file, strerror(errno));
			return EXIT_FAILURE;
		}

		int read = 0;
		int lastread = 0;
		char* readBuffer = (char*) malloc(1000);
		while(true) {
			lastread = fread(readBuffer, 1, 1000, fp);

			if(ferror(fp)) {
				printf("Failed to read from file %s: %s", file, strerror(errno));
				return EXIT_FAILURE;
			}

			if (lastread <= 0)
				break;

			pub->send(readBuffer, lastread);
			read += lastread;

			if (feof(fp))
				break;
		}
		fclose(fp);
		printf("Send %d bytes on channel \"%s\"\n", read, channel);
		if (!interactive)
			exit(0);
	}

	if (sub == NULL) {
		if (protoPath != NULL) {
			sub = new Subscriber(channel, new ProtoBufDumpingReceiver(protoPath));
		} else {
			sub = new Subscriber(channel, new PlainDumpingReceiver());
		}
		node->addSubscriber(sub);
	}

	if (interactive) {
		/**
		 * Enter interactive mode
		 */
		if (pub == NULL) {
			pub = new Publisher(channel);
			node->addPublisher(pub);
		}
		pub->waitForSubscribers(minSubs);
		string line;
		while(std::cin) {
			getline(std::cin, line);
			line.append("\n");
			pub->send(line.c_str(), line.length());
		};
	} else {
		/**
		 * Non-interactive, just let the subscriber print channel messages
		 */
		while (true)
			Thread::sleepMs(500);
	}

}

