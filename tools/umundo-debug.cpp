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

// ./bin/umundo-debug -ofoo.dot && dot -O -Tpdf foo.dot && open foo.dot.pdf
// ./bin/umundo-debug -ofoo.dot && twopi -O -Tpdf foo.dot && open foo.dot.pdf
// ./bin/umundo-debug -ofoo.dot && neato -O -Tpdf foo.dot && open foo.dot.pdf
// ./bin/umundo-debug -ofoo.dot && fdp -O -Tpdf foo.dot && open foo.dot.pdf
// ./bin/umundo-debug -ofoo.dot && sfdp -O -Tpdf foo.dot && open foo.dot.pdf
// ./bin/umundo-debug -ofoo.dot && circo -O -Tpdf foo.dot && open foo.dot.pdf

#define NODE_ENSURE(uuid) \
if (debugNodes.find(uuid) == debugNodes.end()) { \
	debugNodes[uuid] = new DebugNode(); \
}

#define PUB_ENSURE(uuid) \
if (debugPubs.find(uuid) == debugPubs.end()) { \
	debugPubs[uuid] = new DebugPub(); \
}

#define SUB_ENSURE(uuid) \
if (debugSubs.find(uuid) == debugSubs.end()) { \
	debugSubs[uuid] = new DebugSub(); \
}

#define CHECK_AND_ASSIGN(keyCStr, var) \
key = keyCStr; \
if (mIter->substr(0, key.length()) == key) { \
	var = VALUE_FOR_CURR_KEY; \
	continue; \
}

#define VALUE_FOR_CURR_KEY mIter->substr(key.length(), mIter->length() - key.length())

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

// at end of file, rather lengthy
void populateEntities();
std::string timeToDisplay(uint64_t ms);
std::string bytesToDisplay(uint64_t nrBytes);

using namespace umundo;

void* zmq_ctx;
std::ofstream outfile;

std::string uuid;
std::string dotFile;
std::string domain;
bool isQuiet;
size_t waitFor;
uint64_t now;

RMutex mutex;

void printUsageAndExit() {
	printf("umundo-debug version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-debug [-q] [-d domain] [-wN] [-oFILE]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-q                 : do not print debug info messages as they arrive\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-wN                : wait N milli-seconds for nodes (defaults to 3000)\n");
	printf("\t-oFILE             : write umundo layout as a dot file\n");
	printf("\n");
	printf("Example (MacOSX):\n");
	printf("\twhile(true) do umundo-debug -q -w1000 -oumundo.dot && dot -Tpdf -O umundo.dot && open -g umundo.dot.pdf; done\n");
	exit(1);
}


class DebugPub;
class DebugSub;
class DebugNode;

class DebugPub {
public:
	DebugPub() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string type;
	std::string channelName;
	std::string msgsPerSecSent;
	std::string bytesPerSecSent;
	std::map<std::string, DebugNode*> availableAtNode;
	std::map<std::string, DebugNode*> knownByNode;
	std::map<std::string, DebugSub*> connFromSubs;
};

class DebugSub {
public:
	DebugSub() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string type;
	std::string channelName;
	std::map<std::string, DebugNode*> availableAtNode;
	std::map<std::string, DebugNode*> knownByNode;
	std::map<std::string, DebugPub*> connToPubs;
};

class DebugNode {
public:
	DebugNode() : isReal(false) {}
	bool isReal;
	std::string uuid;
	std::string host;
	std::string proc;
	std::string os;
	std::string msgsPerSecSent;
	std::string bytesPerSecSent;
	std::string msgsPerSecRcvd;
	std::string bytesPerSecRcvd;
	std::map<std::string, std::pair<std::string, DebugNode*> > connTo;
	std::map<std::string, DebugNode*> connFrom;
	std::map<std::string, DebugSub*> subs;
	std::map<std::string, DebugPub*> pubs;
};

struct DotNode {
	std::string dotId;
	std::map<std::string, std::string> attr;
	std::map<std::string, std::string> edgeLabel;
};


std::map<std::string, DebugNode*> debugNodes;
std::map<std::string, DebugPub*> debugPubs;
std::map<std::string, DebugSub*> debugSubs;

std::map<std::string, DotNode> dotNodes;
std::map<std::string, DotNode> dotEdges;

std::map<std::string, std::map<std::string, DotNode> > edgeLabels;
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
		uint16_t version = htons(Message::UM_VERSION);
		uint16_t debug = htons(Message::UM_DEBUG);
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
			if (!isQuiet)
				std::cout << data << std::endl;

			messages.push_back(data);

			zmq_getsockopt(_socket, ZMQ_RCVMORE, &more, &more_size) && UM_LOG_ERR("zmq_getsockopt: %s", zmq_strerror(errno));
			zmq_msg_close(&repMsg) && UM_LOG_ERR("zmq_msg_close: %s", zmq_strerror(errno));
			if (!more)
				break;      //  Last message part
		}
		if (!isQuiet)
			std::cout << std::endl;
		if (locked)
			mutex.unlock();
	}

	std::string address;
	void* _socket;

};

std::map<EndPoint, DebugInfoFetcher> endPoints;

class DebugEndPointResultSet : public ResultSet<ENDPOINT_RS_TYPE> {
	void added(ENDPOINT_RS_TYPE ep) {
		// get address into zeromq endpoint address
		std::stringstream address;
		address << ep.getTransport() << "://" << ep.getIP() << ":" << ep.getPort();
		endPoints[ep].address = address.str();
		endPoints[ep].start();
	}

	void removed(ENDPOINT_RS_TYPE ep) {
	}

	void changed(ENDPOINT_RS_TYPE ep, uint64_t what) {
	}
};

void processDebugNode(DebugNode* node) {
	dotNodes[node->uuid].dotId = "\"" + node->uuid + "\"";
	dotNodes[node->uuid].attr["fontsize"] = "8";
	dotNodes[node->uuid].attr["shape"] = "box";
//	dotNodes[node->uuid].attr["style"] = "filled";
//	dotNodes[node->uuid].attr["fillcolor"] = "\"#CCCCCC\"";
	dotNodes[node->uuid].attr["peripheries"] = "2";

	if (node->isReal) {
		dotNodes[node->uuid].attr["color"] = "black";
	} else {
		dotNodes[node->uuid].attr["color"] = "red";
	}

	std::stringstream labelSS;
	labelSS << "<";
	labelSS << "<b>Node[" << SHORT_UUID(node->uuid) << "]</b><br />";
	labelSS << "OS: " << node->os << "<br />";
	if (node->bytesPerSecSent.size() > 0) {
		labelSS << "Sent: " << bytesToDisplay(strTo<uint64_t>(node->bytesPerSecSent)) << "/s in " << node->msgsPerSecSent << "msg/s<br />";
	}
	if (node->bytesPerSecRcvd.size() > 0) {
		labelSS << "Rcvd: " << bytesToDisplay(strTo<uint64_t>(node->bytesPerSecRcvd)) << "/s in " << node->msgsPerSecRcvd << "msg/s<br />";
	}
	labelSS << ">";
	dotNodes[node->uuid].attr["label"] = labelSS.str();

	// polish outgoing edge labels
	if (edgeLabels.find(node->uuid) != edgeLabels.end()) {

		std::map<std::string, DotNode>::iterator eLabelIter = edgeLabels[node->uuid].begin();
		while(eLabelIter != edgeLabels[node->uuid].end()) {

			// remove all edge labels and implied edges to real pubs/subs that are not ours
			if (debugPubs.find(eLabelIter->first) != debugPubs.end() && debugPubs[eLabelIter->first]->isReal) {
				edgeLabels[node->uuid].erase(eLabelIter++);
				continue;
			}
			if (debugSubs.find(eLabelIter->first) != debugSubs.end() && debugSubs[eLabelIter->first]->isReal) {
				edgeLabels[node->uuid].erase(eLabelIter++);
				continue;
			}
			
			DotNode& dotNode = eLabelIter->second;
			if (dotNode.edgeLabel.find("startedAt") != dotNode.edgeLabel.end()) {
				uint64_t msAgo = now - strTo<uint64_t>(dotNode.edgeLabel["startedAt"]);
				dotNode.edgeLabel["startedAt"] = "dscv: " + timeToDisplay(msAgo) + " ago";
			}

			if (dotNode.edgeLabel.find("lastSeen") != dotNode.edgeLabel.end()) {
				uint64_t msAgo = now - strTo<uint64_t>(dotNode.edgeLabel["lastSeen"]);
				dotNode.edgeLabel["lastSeen"] = "seen: " + timeToDisplay(msAgo) + " ago";
			}
			
			eLabelIter++;
		}
	}
	
	std::map<std::string, DebugSub*>::iterator subIter = node->subs.begin();
	while(subIter != node->subs.end()) {
		std::string edgeId = node->uuid + " -> " + subIter->first;
		dotEdges[edgeId].dotId = "\"" + node->uuid + "\" -> \"" + subIter->first + "\"";
		dotEdges[edgeId].attr["arrowhead"] = "diamond";
		dotEdges[edgeId].attr["color"] = "grey";
		dotEdges[edgeId].attr["weight"] = "100";
		subIter++;
	}

	std::map<std::string, DebugPub*>::iterator pubIter = node->pubs.begin();
	while(pubIter != node->pubs.end()) {
		std::string edgeId = node->uuid + " -> " + pubIter->first;
		dotEdges[edgeId].dotId = "\"" + node->uuid + "\" -> \"" + pubIter->first + "\"";
		dotEdges[edgeId].attr["arrowhead"] = "diamond";
		dotEdges[edgeId].attr["color"] = "grey";
		dotEdges[edgeId].attr["weight"] = "100";
		pubIter++;
	}
}

void processDebugPub(DebugPub* pub) {
	dotNodes[pub->uuid].dotId = "\"" + pub->uuid + "\"";
	dotNodes[pub->uuid].attr["fontsize"] = "8";
	dotNodes[pub->uuid].attr["shape"] = "box";

	if (pub->isReal) {
		dotNodes[pub->uuid].attr["color"] = "black";
	} else {
		dotNodes[pub->uuid].attr["color"] = "red";
	}

	std::stringstream labelSS;
	labelSS << "<";
	labelSS << "<b>Pub[" << SHORT_UUID(pub->uuid) << "]</b><br />";
	switch (strTo<uint16_t>(pub->type)) {
	case 1:
		labelSS << "ZMQ";
		break;
	case 2:
		labelSS << "RTP";
		break;
	default:
		labelSS << "UKN";
		break;
	}
	labelSS << "@" << pub->channelName << "<br />";
	labelSS << "#Subscribers: " << pub->connFromSubs.size() << "<br />";
	if (pub->bytesPerSecSent.size() > 0) {
		labelSS << "Sent: " << pub->bytesPerSecSent << "B in " << pub->msgsPerSecSent << "msg/s<br />";
	}

	labelSS << ">";
	dotNodes[pub->uuid].attr["label"] = labelSS.str();

	std::map<std::string, DebugSub*>::iterator subIter = pub->connFromSubs.begin();
	while(subIter != pub->connFromSubs.end()) {
		std::string edgeId = pub->uuid + " -> " + subIter->first;
		dotEdges[edgeId].dotId = "\"" + pub->uuid + "\" -> \"" + subIter->first + "\"";
		dotEdges[edgeId].attr["arrowhead"] = "normal";
		dotEdges[edgeId].attr["color"] = "black";
		subIter++;
	}
}

void processDebugSub(DebugSub* sub) {
	dotNodes[sub->uuid].dotId = "\"" + sub->uuid + "\"";
	dotNodes[sub->uuid].attr["fontsize"] = "8";
	dotNodes[sub->uuid].attr["shape"] = "box";

	if (sub->isReal) {
		dotNodes[sub->uuid].attr["color"] = "black";
	} else {
		dotNodes[sub->uuid].attr["color"] = "red";
	}

	std::stringstream labelSS;
	labelSS << "<";
	labelSS << "<b>Sub[" << SHORT_UUID(sub->uuid) << "]</b><br />";
	switch (strTo<uint16_t>(sub->type)) {
	case 1:
		labelSS << "ZMQ";
		break;
	case 2:
		labelSS << "RTP";
		break;
	default:
		labelSS << "UKN";
		break;
	}
	labelSS << "@" << sub->channelName << "<br />";
	labelSS << "#Publishers: " << sub->connToPubs.size() << "<br />";
	labelSS << ">";
	dotNodes[sub->uuid].attr["label"] = labelSS.str();

	std::map<std::string, DebugPub*>::iterator pubIter = sub->connToPubs.begin();
	while(pubIter != sub->connToPubs.end()) {
		std::string edgeId = sub->uuid + " -> " + pubIter->first;
		std::string backEdgeId = pubIter->first + " -> " + sub->uuid;
		if (dotEdges.find(backEdgeId) != dotEdges.end()) {
			dotEdges[backEdgeId].attr["dir"] = "both";
		} else {
			dotEdges[edgeId].dotId = "\"" + sub->uuid + "\" -> \"" + pubIter->first + "\"";
			dotEdges[edgeId].attr["arrowhead"] = "normal";
			dotEdges[edgeId].attr["color"] = "black";
		}
		pubIter++;
	}
}


void generateDotFile() {
	populateEntities();
	
	// process into edges and nodes for dot
	std::map<std::string, DebugNode*>::iterator nodeIter = debugNodes.begin();
	while(nodeIter != debugNodes.end()) {
		processDebugNode(nodeIter->second);
		nodeIter++;
	}
	
	std::map<std::string, DebugPub*>::iterator pubIter = debugPubs.begin();
	while(pubIter != debugPubs.end()) {
		processDebugPub(pubIter->second);
		pubIter++;
	}

	std::map<std::string, DebugSub*>::iterator subIter = debugSubs.begin();
	while(subIter != debugSubs.end()) {
		processDebugSub(subIter->second);
		subIter++;
	}

	// do we have some more labels or attributs for edges?
	std::map<std::string, std::map<std::string, DotNode> >::iterator fromIter = edgeLabels.begin();
	while (fromIter != edgeLabels.end()) {
		std::map<std::string, DotNode>& moreLabels = fromIter->second;
		std::map<std::string, DotNode>::iterator toIter = moreLabels.begin();
		while(toIter != moreLabels.end()) {
			std::string edgeId = fromIter->first + " -> " + toIter->first;
			dotEdges[edgeId].dotId = "\"" + fromIter->first + "\" -> \"" + toIter->first + "\"";
			dotEdges[edgeId].edgeLabel.insert(toIter->second.edgeLabel.begin(), toIter->second.edgeLabel.end());
			dotEdges[edgeId].attr.insert(toIter->second.attr.begin(), toIter->second.attr.end());
			toIter++;
		}
		fromIter++;
	}

	// now write it as a dot file
	outfile << "digraph {" << std::endl;
	outfile << "rankdir=LR; fontsize=10;" << std::endl;

	// process dot nodes
	std::map<std::string, DotNode>::iterator dNodeIter = dotNodes.begin();
	while(dNodeIter != dotNodes.end()) {
		outfile << dNodeIter->second.dotId;
		outfile << "[";

		std::map<std::string, std::string>::iterator attrIter = dNodeIter->second.attr.begin();
		std::string seperator;
		while (attrIter != dNodeIter->second.attr.end()) {
			outfile << seperator << attrIter->first << "=" << attrIter->second;
			seperator = ",";
			attrIter++;
		}

		outfile << "];" << std::endl;
		dNodeIter++;
	}

	// process dot edges
	std::map<std::string, DotNode>::iterator dEdgeIter = dotEdges.begin();
	while(dEdgeIter != dotEdges.end()) {
		outfile << dEdgeIter->second.dotId;
		outfile << " [";

		if (dEdgeIter->second.attr.find("fontsize") == dEdgeIter->second.attr.end()) {
			dEdgeIter->second.attr["fontsize"] = "8";
		}
		
		std::map<std::string, std::string>::iterator attrIter = dEdgeIter->second.attr.begin();
		std::string seperator;
		while (attrIter != dEdgeIter->second.attr.end()) {
			outfile << seperator << attrIter->first << "=" << attrIter->second;
			seperator = ",";
			attrIter++;
		}

		// we have a label?
		std::stringstream labelSS;
		std::string labelSep;
		std::map<std::string, std::string>::iterator labelIter = dEdgeIter->second.edgeLabel.begin();
		while (labelIter != dEdgeIter->second.edgeLabel.end()) {
			labelSS << labelSep << labelIter->second;
			labelSep = "<br />";
			labelIter++;
		}
		if (labelSS.str().size() > 0) {
			outfile << seperator << "label=<" << labelSS.str() << ">";
		}
		outfile << "];" << std::endl;
		dEdgeIter++;
	}

	outfile << "}" << std::endl;
}

int main(int argc, char** argv) {

	waitFor = 3000;
	isQuiet = false;

	int option;
	while ((option = getopt(argc, argv, "d:w:o:q")) != -1) {
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
		case 'q':
			isQuiet = true;
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
	now = Thread::getTimeStampMs();
	
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

void populateEntities() {
	// iterate all messages
	std::list<std::string>::iterator mIter;
	
	DebugNode* currNode = NULL;
	DebugPub* currPub = NULL;
	DebugSub* currSub = NULL;
	
	DebugNode* currRemoteNode = NULL;
	DebugPub* currRemotePub = NULL;
	DebugSub* currRemoteSub = NULL;
	
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
			std::string uuid = VALUE_FOR_CURR_KEY;
			NODE_ENSURE(uuid)
			currNode = debugNodes[uuid];
			currNode->uuid = uuid;
			currNode->isReal = true;
			continue;
		}
		
		if (!currNode) // we cannot continue until we have a node - we rely on the order returned by nodes
			continue;
		
		key = "done:";
		if (mIter->substr(0, key.length()) == key) {
			currNode = NULL;
			continue;
		}
		
		CHECK_AND_ASSIGN("host:", currNode->host);
		CHECK_AND_ASSIGN("os:", currNode->os);
		CHECK_AND_ASSIGN("proc:", currNode->proc);
		CHECK_AND_ASSIGN("sent:msgs:", currNode->msgsPerSecSent);
		CHECK_AND_ASSIGN("sent:bytes:", currNode->bytesPerSecSent);
		CHECK_AND_ASSIGN("rcvd:msgs:", currNode->msgsPerSecRcvd);
		CHECK_AND_ASSIGN("rcvd:bytes:", currNode->bytesPerSecRcvd);
		
		// process publishers
		key = "pub:";
		if (mIter->substr(0, key.length()) == key) {
			
			key = "pub:uuid:";
			if (mIter->substr(0,key.length()) == key) {
				std::string uuid = VALUE_FOR_CURR_KEY;
				PUB_ENSURE(uuid)
				currPub = debugPubs[uuid];
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
			CHECK_AND_ASSIGN("pub:sent:msgs:", currPub->msgsPerSecSent);
			CHECK_AND_ASSIGN("pub:sent:bytes:", currPub->bytesPerSecSent);
			
			// remote sub registered at the publisher
			key = "pub:sub";
			if (mIter->substr(0, key.length()) == key) {
				
				key = "pub:sub:uuid:";
				if (mIter->substr(0,key.length()) == key) {
					std::string uuid = VALUE_FOR_CURR_KEY;
					SUB_ENSURE(uuid)
					currRemoteSub = debugSubs[uuid];
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
				std::string uuid = VALUE_FOR_CURR_KEY;
				SUB_ENSURE(uuid)
				currSub = debugSubs[uuid];
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
					std::string uuid = VALUE_FOR_CURR_KEY;
					PUB_ENSURE(uuid)
					currRemotePub = debugPubs[uuid];
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
				std::string uuid = VALUE_FOR_CURR_KEY;
				NODE_ENSURE(uuid)
				currRemoteNode = debugNodes[uuid];
				currRemoteNode->uuid = uuid;
				continue;
			}
			
			if (!currRemoteNode)
				continue;
			
			key = "conn:to:1";
			if (mIter->substr(0, key.length()) == key) {
				currNode->connTo[currRemoteNode->uuid].second = currRemoteNode;
				continue;
			}
			
			key = "conn:address:";
			if (mIter->substr(0, key.length()) == key) {
				edgeLabels[currNode->uuid][currRemoteNode->uuid].edgeLabel["address"] = VALUE_FOR_CURR_KEY;
				continue;
			}
			
			key = "conn:confirmed:";
			if (mIter->substr(0, key.length()) == key) {
				if (!strTo<bool>(VALUE_FOR_CURR_KEY)) {
					edgeLabels[currNode->uuid][currRemoteNode->uuid].attr["style"] = "dashed";
				}
				continue;
			}
			
			key = "conn:startedAt:";
			if (mIter->substr(0, key.length()) == key) {
				edgeLabels[currNode->uuid][currRemoteNode->uuid].edgeLabel["startedAt"] = VALUE_FOR_CURR_KEY;
				continue;
			}
			
			key = "conn:lastSeen:";
			if (mIter->substr(0, key.length()) == key) {
				edgeLabels[currNode->uuid][currRemoteNode->uuid].edgeLabel["lastSeen"] = VALUE_FOR_CURR_KEY;
				continue;
			}
			
			key = "conn:from:1";
			if (mIter->substr(0, key.length()) == key) {
				currNode->connFrom[currRemoteNode->uuid] = currRemoteNode;
				continue;
			}
			
			// process remote publishers
			key = "conn:pub:";
			if (mIter->substr(0, key.length()) == key) {
				
				key = "conn:pub:uuid:";
				if (mIter->substr(0, key.length()) == key) {
					PUB_ENSURE(VALUE_FOR_CURR_KEY);
					currRemotePub = debugPubs[VALUE_FOR_CURR_KEY];
					currRemotePub->uuid = VALUE_FOR_CURR_KEY;
					edgeLabels[currNode->uuid][VALUE_FOR_CURR_KEY].edgeLabel["uuid"] = VALUE_FOR_CURR_KEY.substr(0,6);
					continue;
				}
				
				key = "conn:pub:channelName:";
				if (mIter->substr(0, key.length()) == key) {
					edgeLabels[currNode->uuid][currRemotePub->uuid].edgeLabel["channelName"] = VALUE_FOR_CURR_KEY;
					continue;
				}
				
				key = "conn:pub:type:";
				if (mIter->substr(0, key.length()) == key) {
					edgeLabels[currNode->uuid][currRemotePub->uuid].edgeLabel["type"] = VALUE_FOR_CURR_KEY;
					continue;
				}
			}
			
			// process remote subscribers
			key = "conn:sub:";
			if (mIter->substr(0, key.length()) == key) {
				
				key = "conn:sub:uuid:";
				if (mIter->substr(0, key.length()) == key) {
					SUB_ENSURE(VALUE_FOR_CURR_KEY);
					currRemoteSub = debugSubs[VALUE_FOR_CURR_KEY];
					currRemoteSub->uuid = VALUE_FOR_CURR_KEY;
					edgeLabels[currNode->uuid][VALUE_FOR_CURR_KEY].edgeLabel["uuid"] = VALUE_FOR_CURR_KEY.substr(0,6);
					continue;
				}
				
				key = "conn:sub:channelName:";
				if (mIter->substr(0, key.length()) == key) {
					edgeLabels[currNode->uuid][currRemoteSub->uuid].edgeLabel["channelName"] = VALUE_FOR_CURR_KEY;
					continue;
				}
				
				key = "conn:sub:type:";
				if (mIter->substr(0, key.length()) == key) {
					edgeLabels[currNode->uuid][currRemoteSub->uuid].edgeLabel["type"] = VALUE_FOR_CURR_KEY;
					continue;
				}
				
			}
			
		}
		std::cout << "Debug information '" << *mIter << "' unhandled" << std::endl;
	}
}

std::string bytesToDisplay(uint64_t nrBytes) {
	std::stringstream ss;
	ss << std::setprecision(5);
	
	uint64_t kMax = (uint64_t)1024 * (uint64_t)1024;
	uint64_t mMax = kMax * (uint64_t)1024;
	uint64_t gMax = mMax * (uint64_t)1024;
	
	if (nrBytes < 1024) {
		ss << nrBytes << "B";
		return ss.str();
	}
	if (nrBytes < kMax) {
		ss << ((double)nrBytes / (double)1024) << "KB";
		return ss.str();
	}
	if (nrBytes < mMax) {
		ss << ((double)nrBytes / (double)kMax) << "MB";
		return ss.str();
	}
	if (nrBytes < gMax) {
		ss << ((double)nrBytes / (double)mMax) << "GB";
		return ss.str();
	}
	ss << ((double)nrBytes / (double)gMax) << "TB";
	return ss.str();
}

std::string timeToDisplay(uint64_t ms) {
	std::stringstream ss;
	ss << std::setprecision(4);
	
	if (ms < 1000) {
		ss << ms << "ms";
		return ss.str();
	}
	if (ms < 1000 * 60) {
		ss << ((double)ms / (double)1000) << "s";
		return ss.str();
	}
	if (ms < 1000 * 60 * 60) {
		ss << ((double)ms / (double)(1000 * 60)) << "min";
		return ss.str();
	}
	ss << ((double)ms / (double)(1000 * 60 * 60)) << "h";
	return ss.str();
}

