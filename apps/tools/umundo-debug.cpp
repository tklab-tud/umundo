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

#define NODE_ENSURE(uuid) \
if (nodeDots.find(uuid) == nodeDots.end()) { \
	nodeDots[uuid] = new NodeDot(); \
}

#define PUB_ENSURE(uuid) \
if (pubDots.find(uuid) == pubDots.end()) { \
	pubDots[uuid] = new PubDot(); \
}

#define SUB_ENSURE(uuid) \
if (subDots.find(uuid) == subDots.end()) { \
	subDots[uuid] = new SubDot(); \
}

#define CHECK_AND_ASSIGN(keyCStr, var) \
key = keyCStr; \
if (mIter->substr(0, key.length()) == key) { \
	var = mIter->substr(key.length(), mIter->length() - key.length()); \
	continue; \
}


#include "umundo/config.h"
#include "umundo/core.h"
#include "umundo/s11n.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <zmq.h>

#ifdef WIN32
#include "XGetopt.h"
#endif

#if defined UNIX
#include <arpa/inet.h>
#endif

#ifdef WIN32
std::string pathSeperator = "\\";
#else
std::string pathSeperator = "/";
#endif

using namespace umundo;

void* zmq_ctx;
std::ofstream outfile;

std::string uuid;
std::string dotFile;
std::string domain;
bool isVerbose;
size_t waitFor;

Mutex mutex;

void printUsageAndExit() {
	printf("umundo-debug version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-debug [-v] [-d domain] [-wN] [-oFILE]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-v                 : print debug info messages as the arrive\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-wN                : wait N milli-seconds for nodes (defaults to 3000)\n");
	printf("\t-oFILE             : write umundo layout as a dot file\n");
	exit(1);
}

class PubDot;
class SubDot;
class NodeDot;

class PubDot {
public:
	PubDot() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string type;
	std::string channelName;
	std::map<std::string, NodeDot*> availableAtNode;
	std::map<std::string, NodeDot*> knownByNode;
	std::map<std::string, SubDot*> connFromSubs;
};

class SubDot {
public:
	SubDot() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string type;
	std::string channelName;
	std::map<std::string, NodeDot*> availableAtNode;
	std::map<std::string, NodeDot*> knownByNode;
	std::map<std::string, PubDot*> connToPubs;
};

class NodeDot {
public:
	NodeDot() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string host;
	std::string proc;
	std::map<std::string, NodeDot*> connTo;
	std::map<std::string, NodeDot*> connFrom;
	std::map<std::string, SubDot*> subs;
	std::map<std::string, PubDot*> pubs;
};

std::map<std::string, NodeDot*> nodeDots;
std::map<std::string, PubDot*> pubDots;
std::map<std::string, SubDot*> subDots;
std::list<std::string> messages;

class DebugInfoFetcher : public Thread {
public:

	virtual void run() {
		int recvTimeout = waitFor;
		size_t recvTimeoutSize = sizeof(recvTimeout);
		// connect
		(_socket = zmq_socket(zmq_ctx, ZMQ_REQ))  || UM_LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
		zmq_setsockopt(_socket, ZMQ_IDENTITY, uuid.c_str(), uuid.length()) && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
		zmq_setsockopt(_socket, ZMQ_RCVTIMEO, &recvTimeout, recvTimeoutSize) && UM_LOG_ERR("zmq_setsockopt: %s", zmq_strerror(errno));
		zmq_connect(_socket, address.c_str()) && UM_LOG_ERR("zmq_connect %s: %s", address.c_str(), zmq_strerror(errno));

		// prepare a debug request
		char buf[4];
		uint16_t version = htons(Message::VERSION);
		uint16_t debug = htons(Message::DEBUG);
		buf[0] = (version >> 0) & 0xff;
		buf[1] = (version >> 8) & 0xff;
		buf[2] = (debug >> 0) & 0xff;
		buf[3] = (debug >> 8) & 0xff;

		// send debug request
		zmq_send(_socket, buf, 4, 0) == -1 && UM_LOG_ERR("zmq_send: %s", zmq_strerror(errno));

		zmq_msg_t repMsg;
		int more;
		size_t more_size = sizeof(more);
		size_t msgSize;
		bool locked = false;

		// receive reply and read envelope
		while (1) {
			//  Process all parts of the message
			zmq_msg_init(&repMsg) && UM_LOG_ERR("zmq_msg_init: %s", zmq_strerror(errno));
			int rc = zmq_msg_recv(&repMsg, _socket, 0);
			if (rc == -1)
				break;

			if (!locked)
				mutex.lock();
			locked = true;

			msgSize = zmq_msg_size(&repMsg);

			std::string data((char*)zmq_msg_data(&repMsg), msgSize);
			if (isVerbose)
				std::cout << data << std::endl;

			messages.push_back(data);

			zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
			zmq_msg_close(&repMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
			if (!more)
				break;      //  Last message part
		}
		std::cout << std::endl;
		if (locked)
			mutex.unlock();
	}

	std::string address;
	void* _socket;

};

std::map<EndPoint, DebugInfoFetcher> endPoints;

class DebugEndPointResultSet : public ResultSet<EndPoint> {
	void added(EndPoint ep) {
		// get address into zeromq endpoint address
		std::stringstream address;
		address << ep.getTransport() << "://" << ep.getIP() << ":" << ep.getPort();
		endPoints[ep].address = address.str();
		endPoints[ep].start();
	}

	void removed(EndPoint ep) {
	}

	void changed(EndPoint ep) {
	}
};

void writeNodeDot(NodeDot* node) {
	outfile << "\"" << node->uuid << "\" [";
	outfile << "fontsize=8,";
	outfile << "shape=box,";
	if (node->isReal) {
		outfile << "color=black,";
	} else {
		outfile << "color=red,";
	}
	outfile << "label=<";
	outfile << "<b>Node[" << SHORT_UUID(node->uuid) << "]</b><br />";
	outfile << ">";
	outfile << "];" << std::endl;

	std::map<std::string, SubDot*>::iterator subIter = node->subs.begin();
	while(subIter != node->subs.end()) {
		outfile << "\"" << node->uuid << "\" -> \"" << subIter->first << "\" [";
		outfile << "arrowhead=normal,";
		outfile << "color=black";
		outfile << "];" << std::endl;
		subIter++;
	}

	std::map<std::string, PubDot*>::iterator pubIter = node->pubs.begin();
	while(pubIter != node->pubs.end()) {
		outfile << "\"" << node->uuid << "\" -> \"" << pubIter->first << "\" [";
		outfile << "arrowhead=normal,";
		outfile << "color=black";
		outfile << "];" << std::endl;
		pubIter++;
	}

}

void writePubDot(PubDot* pub) {
	outfile << "\"" << pub->uuid << "\" [";
	outfile << "fontsize=8,";
	outfile << "shape=box,";
	if (pub->isReal) {
		outfile << "color=black,";
	} else {
		outfile << "color=red,";
	}
	outfile << "label=<";
	outfile << "<b>Pub[" << SHORT_UUID(pub->uuid) << "]</b><br />";
	switch (strTo<uint16_t>(pub->type)) {
	case 1:
		outfile << "ZMQ";
		break;
	case 2:
		outfile << "RTP";
		break;
	default:
		outfile << "UKN";
		break;
	}
	outfile << "@" << pub->channelName << "<br />";
	outfile << ">";
	outfile << "];" << std::endl;

	std::map<std::string, SubDot*>::iterator subIter = pub->connFromSubs.begin();
	while(subIter != pub->connFromSubs.end()) {
		outfile << "\"" << pub->uuid << "\" -> \"" << subIter->first << "\" [";
		outfile << "arrowhead=normal,";
		outfile << "color=black";
		outfile << "];" << std::endl;
		subIter++;
	}
}

void writeSubDot(SubDot* sub) {
	outfile << "\"" << sub->uuid << "\" [";
	outfile << "fontsize=8,";
	outfile << "shape=box,";
	if (sub->isReal) {
		outfile << "color=black,";
	} else {
		outfile << "color=red,";
	}
	outfile << "label=<";
	outfile << "<b>Sub[" << SHORT_UUID(sub->uuid) << "]</b><br />";
	switch (strTo<uint16_t>(sub->type)) {
	case 1:
		outfile << "ZMQ";
		break;
	case 2:
		outfile << "RTP";
		break;
	default:
		outfile << "UKN";
		break;
	}
	outfile << "@" << sub->channelName << "<br />";
	outfile << ">";
	outfile << "];" << std::endl;

	std::map<std::string, PubDot*>::iterator pubIter = sub->connToPubs.begin();
	while(pubIter != sub->connToPubs.end()) {
		outfile << "\"" << sub->uuid << "\" -> \"" << pubIter->first << "\" [";
		outfile << "arrowhead=normal,";
		outfile << "color=black";
		outfile << "];" << std::endl;
		pubIter++;
	}
}

void generateDotFile() {
	// iterate all messages
	std::list<std::string>::iterator mIter;

	NodeDot* currNode = NULL;
	PubDot* currPub = NULL;
	SubDot* currSub = NULL;

	NodeDot* currRemoteNode = NULL;
	PubDot* currRemotePub = NULL;
	SubDot* currRemoteSub = NULL;

	for(mIter = messages.begin(); mIter != messages.end(); mIter++) {
		std::string key;
		if (mIter->length() == 0) {
			currNode = NULL;
			currPub = NULL;
			currSub = NULL;
		}

		// assume that we have a uuid to read
		key = "uuid:";
		if (mIter->substr(0, key.length()) == key) {
			std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
			NODE_ENSURE(uuid)
			currNode = nodeDots[uuid];
			currNode->uuid = uuid;
			currNode->isReal = true;
			continue;
		}

		if (!currNode) // we cannot continue until we have a node
			continue;

		CHECK_AND_ASSIGN("host:", currNode->host);
		CHECK_AND_ASSIGN("proc:", currNode->proc);

		// process publishers
		key = "pub:";
		if (mIter->substr(0, key.length()) == key) {

			key = "pub:uuid:";
			if (mIter->substr(0,key.length()) == key) {
				std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
				PUB_ENSURE(uuid)
				currPub = pubDots[uuid];
				currPub->uuid = uuid;
				currPub->isReal = true;
				currPub->availableAtNode[currNode->uuid] = currNode;
				currNode->pubs[uuid] = currPub;
				continue;
			}

			if (!currPub)
				continue;

			CHECK_AND_ASSIGN("pub:channelName:", currPub->channelName);
			CHECK_AND_ASSIGN("pub:type:", currPub->type);

			// remote sub registered at the publisher
			key = "pub:sub";
			if (mIter->substr(0, key.length()) == key) {

				key = "pub:sub:uuid:";
				if (mIter->substr(0,key.length()) == key) {
					std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
					SUB_ENSURE(uuid)
					currRemoteSub = subDots[uuid];
					currRemoteSub->uuid = uuid;
					currRemoteSub->knownByNode[currNode->uuid] = currNode;
					currPub->connFromSubs[uuid] = currRemoteSub;
					continue;
				}

				if (!currRemoteSub)
					continue;

				CHECK_AND_ASSIGN("pub:sub:channelName:", currRemoteSub->channelName);
				CHECK_AND_ASSIGN("pub:sub:type:", currRemoteSub->type);

			}
		}


		// process subscribers
		key = "sub:";
		if (mIter->substr(0, key.length()) == key) {

			key = "sub:uuid:";
			if (mIter->substr(0,key.length()) == key) {
				std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
				SUB_ENSURE(uuid)
				currSub = subDots[uuid];
				currSub->uuid = uuid;
				currSub->isReal = true;
				currSub->availableAtNode[currNode->uuid] = currNode;
				currNode->subs[uuid] = currSub;
				continue;
			}

			if (!currSub)
				continue;

			CHECK_AND_ASSIGN("sub:channelName:", currSub->channelName);
			CHECK_AND_ASSIGN("sub:type:", currSub->type);

			// remote pub registered at the subscriber
			key = "sub:pub";
			if (mIter->substr(0, key.length()) == key) {

				key = "sub:pub:uuid:";
				if (mIter->substr(0,key.length()) == key) {
					std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
					PUB_ENSURE(uuid)
					currRemotePub = pubDots[uuid];
					currRemotePub->uuid = uuid;
					currRemotePub->knownByNode[currNode->uuid] = currNode;
					currSub->connToPubs[uuid] = currRemotePub;
					continue;
				}

				if (!currRemoteSub)
					continue;

				CHECK_AND_ASSIGN("sub:pub:channelName:", currRemotePub->channelName);
				CHECK_AND_ASSIGN("sub:pub:type:", currRemotePub->type);

			}
		}

		// process connections
		key = "conn:";
		if (mIter->substr(0, key.length()) == key) {

			key = "conn:uuid:";
			if (mIter->substr(0,key.length()) == key) {
				std::string uuid = mIter->substr(key.length(), mIter->length() - key.length());
				NODE_ENSURE(uuid)
				currRemoteNode = nodeDots[uuid];
				currRemoteNode->uuid = uuid;
				continue;
			}

			if (!currRemoteNode)
				continue;

			key = "conn:from:1";
			if (mIter->substr(0, key.length()) == key) {
				currNode->connTo[currRemoteNode->uuid] = currRemoteNode;
				currRemoteNode->connFrom[currNode->uuid] = currNode;
			}

			key = "conn:to:1";
			if (mIter->substr(0, key.length()) == key) {
				currNode->connFrom[currRemoteNode->uuid] = currRemoteNode;
				currRemoteNode->connTo[currNode->uuid] = currNode;
			}

		}
	}

	// now write it as a dot file
	outfile << "digraph {" << std::endl;
	outfile << "rankdir=TB; fontsize=10;" << std::endl;

	std::map<std::string, NodeDot*>::iterator nodeIter = nodeDots.begin();
	while(nodeIter != nodeDots.end()) {
		writeNodeDot(nodeIter->second);
		nodeIter++;
	}
	std::map<std::string, PubDot*>::iterator pubIter = pubDots.begin();
	while(pubIter != pubDots.end()) {
		writePubDot(pubIter->second);
		pubIter++;
	}

	std::map<std::string, SubDot*>::iterator subIter = subDots.begin();
	while(subIter != subDots.end()) {
		writeSubDot(subIter->second);
		subIter++;
	}


	outfile << "}" << std::endl;
}

int main(int argc, char** argv) {

	waitFor = 3000;
	isVerbose = false;

	int option;
	while ((option = getopt(argc, argv, "d:w:o:v")) != -1) {
		switch(option) {
		case 'd':
			domain = optarg;
			break;
		case 'w':
			waitFor = strTo<size_t>(optarg);
			break;
		case 'o':
			dotFile = optarg;
			break;
		case 'v':
			isVerbose = true;
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	zmq_ctx = zmq_ctx_new();
	uuid = UUID::getUUID();

	Discovery disc(Discovery::MDNS, domain);
	DebugEndPointResultSet debugRS;
	disc.browse(&debugRS);

	Thread::sleepMs(waitFor);

	disc.unbrowse(&debugRS);

	if (dotFile.size() > 0) {
		outfile.open(dotFile.c_str());
		if (!outfile) {
			printf("Failed to open file %s: %s\n", dotFile.c_str(), strerror(errno));
			return EXIT_FAILURE;
		}

		generateDotFile();
		outfile.close();
	}
}

