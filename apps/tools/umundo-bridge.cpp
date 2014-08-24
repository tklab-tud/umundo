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
#include "umundo/thread/Thread.h"
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
	printf("\n");
	printf("Example\n");
	
	printf("\tumundo-bridge tcp://130.32.14.22:4242 tcp://53.2.23.72:4242\n");
	exit(1);
}

//class for simple handling and output of bridge node/endpoint type (local, remote)
class BridgeType {
public:
	enum dummy { LOCAL, REMOTE };
	BridgeType(dummy type) : _type(type) { }
	BridgeType operator!() {
		switch(_type) {
			case BridgeType::LOCAL: return BridgeType(REMOTE);
			case BridgeType::REMOTE: return BridgeType(LOCAL);
		}
		return BridgeType(LOCAL);
	}
	bool operator==(const BridgeType& other) const {
		return other._type==_type;
	}
private:
	dummy _type;
};
std::ostream& operator<<(std::ostream& os, BridgeType type) {
	if(type==BridgeType::LOCAL)
		os << "LOCAL";
	else if(type==BridgeType::REMOTE)
		os << "REMOTE";
	return os;
}

//output of publisher or subscriber
std::ostream& operator<<(std::ostream& os, Publisher& pub) {
	EndPoint endPoint = dynamic_cast<EndPoint&>(pub);
	os << pub.getUUID() << " channel '" << pub.getChannelName() << "' (" << endPoint << ")";
	return os;
}
std::ostream& operator<<(std::ostream& os, Subscriber& sub) {
	EndPoint endPoint = dynamic_cast<EndPoint&>(sub);
	os << sub.getUUID() << " channel '" << sub.getChannelName() << "' (" << endPoint << ")";
	return os;
}
std::ostream& operator<<(std::ostream& os, PublisherStub& pubStub) {
	EndPoint endPoint = dynamic_cast<EndPoint&>(pubStub);
	os << pubStub.getUUID() << " channel '" << pubStub.getChannelName() << "' (" << endPoint << ")";
	return os;
}
std::ostream& operator<<(std::ostream& os, SubscriberStub& subStub) {
	EndPoint endPoint = dynamic_cast<EndPoint&>(subStub);
	os << subStub.getUUID() << " channel '" << subStub.getChannelName() << "' (" << endPoint << ")";
	return os;
}

//transfer incoming messages from one node to the other
class GlobalReceiver : public Receiver {
private:
	Publisher* _pub;
	BridgeType _type;
	
public:
	GlobalReceiver(Publisher* pub, BridgeType type) : _pub(pub), _type(type) { }
	
	void receive(Message* msg) {
		if(verbose)
			std::cout << "Forwarding message using " << _type << " publisher " << *_pub << std::endl;
		_pub->send(msg);
	}
};

//monitor subscriptions to our publishers and subscribe on the other side if neccessary
class GlobalGreeter : public Greeter {
private:
	Node& _receiverNode;
	Publisher* _pub;
	BridgeType _type;
	std::map<std::string, Subscriber*> _knownSubs;
	RMutex _mutex;
	
public:
	GlobalGreeter(Node& receiverNode, Publisher* pub, BridgeType type) : _receiverNode(receiverNode), _pub(pub), _type(type) { }
	
	void welcome(Publisher& pub, const SubscriberStub& subStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got new " << _type << " subscriber: " << subStub << " to publisher " << *_pub << " --> subscribing on " << !_type << " side..." << std::endl;
		GlobalReceiver* receiver = new GlobalReceiver(_pub, _type);
		Subscriber* sub = new Subscriber(_pub->getChannelName(), receiver);
		_knownSubs[subStub.getUUID()] = sub;
		_receiverNode.addSubscriber(*sub);
		if(verbose)
			std::cout << "Created new " << !_type << " subscriber " << sub->getUUID() << " for " << _type << " subscriber " << subStub.getUUID() << std::endl;
	}
	
	void farewell(Publisher& pub, const SubscriberStub& subStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Removed " << _type << " subscriber: " << subStub << " to publisher " << *_pub << " --> unsubscribing on " << !_type << " side..." << std::endl;
		if(_knownSubs.find(subStub.getUUID())==_knownSubs.end())
		{
			if(verbose)
				std::cout << _type << " subscriber " << subStub << " unknown, ignoring removal!" << std::endl;
			return;
		}
		std::string localUUID = _knownSubs[subStub.getUUID()]->getUUID();
		this->unsubscribe(subStub.getUUID());
		_knownSubs.erase(subStub.getUUID());
		if(verbose)
			std::cout << "Removed " << !_type << " subscriber " << localUUID << " for " << _type << " subscriber " << subStub.getUUID() << std::endl;
	}
	
	void unsubscribeAll() {
		std::map<std::string, Subscriber*>::iterator subIter = _knownSubs.begin();
		while(subIter != _knownSubs.end())
		{
			this->unsubscribe(subIter->first);
			subIter++;
		}
		_knownSubs.clear();
	}
	
	void unsubscribe(std::string subUUID) {
		Receiver* receiver = _knownSubs[subUUID]->getReceiver();
		_knownSubs[subUUID]->setReceiver(NULL);
		_receiverNode.removeSubscriber(*_knownSubs[subUUID]);
		delete _knownSubs[subUUID];
		delete receiver;
	}
};

//monitor addition and removal of new publishers and publish the same channel on the other side
class PubMonitor : public ResultSet<PublisherStub> {
private:
	Node& _receiverNode;
	Node& _senderNode;
	std::map<std::string, Publisher*> _knownPubs;
	BridgeType _type;
	RMutex _mutex;
	
public:
	PubMonitor(Node& receiverNode, Node& senderNode, BridgeType type) : _receiverNode(receiverNode), _senderNode(senderNode), _type(type) { }
	
	void added(PublisherStub pubStub) {
		if(pubStub.getChannelName().substr(0, 13)=="umundo-bridge")		//ignore bridge internal publishers
			return;
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got new " << _type << " publisher: " << pubStub << " --> publishing on " << !_type << " side..." << std::endl;
		if(_knownPubs.find(pubStub.getUUID())!=_knownPubs.end())
		{
			if(verbose)
				std::cout << _type << " publisher " << pubStub << " already known, ignoring!" << std::endl;
			return;
		}
		Publisher* ownPub = new Publisher(pubStub.getChannelName());
		GlobalGreeter* ownGreeter = new GlobalGreeter(_receiverNode, ownPub, !_type);
		ownPub->setGreeter(ownGreeter);
		_knownPubs[pubStub.getUUID()] = ownPub;
		_senderNode.addPublisher(*ownPub);
		if(verbose)
			std::cout << "Created new " << !_type << " publisher " << ownPub->getUUID() << " for " << _type << " publisher " << pubStub.getUUID() << std::endl;
	}
	
	void removed(PublisherStub pubStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got removal of " << _type << " publisher: " << pubStub << " --> also removing on " << !_type << " side..." << std::endl;
		if(_knownPubs.find(pubStub.getUUID())==_knownPubs.end())
		{
			if(verbose)
				std::cout << _type << " publisher " << pubStub << " unknown, ignoring removal!" << std::endl;
			return;
		}
		std::string localUUID = _knownPubs[pubStub.getUUID()]->getUUID();
		GlobalGreeter* greeter = dynamic_cast<GlobalGreeter*>(_knownPubs[pubStub.getUUID()]->getGreeter());
		_knownPubs[pubStub.getUUID()]->setGreeter(NULL);
		greeter->unsubscribeAll();
		_senderNode.removePublisher(*_knownPubs[pubStub.getUUID()]);
		_knownPubs.erase(pubStub.getUUID());
		delete _knownPubs[pubStub.getUUID()];
		delete greeter;
		if(verbose)
			std::cout << "Removed " << !_type << " publisher " << localUUID << " for " << _type << " publisher " << pubStub.getUUID() << std::endl;
	}
	
	void changed(PublisherStub pubStub) {
		//do nothing here
	}
};

class PingReceiver : public Receiver {
public:
	PingReceiver() {};
	void receive(Message* msg) {
		if(verbose)
			std::cout << "i" << std::flush;
	}
};

int main(int argc, char** argv) {
	int option;
	while((option = getopt(argc, argv, "vd:")) != -1) {
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
	
	if(optind >= argc - 1)
		printUsageAndExit();
	
	std::string localAddress = argv[optind];
	std::string remoteAddress = argv[optind + 1];
	
	EndPoint endPoint(remoteAddress);
	if(!endPoint)
		printUsageAndExit();
	
	//construct outer and inner nodes
	Node outerNode(localAddress);
	Node innerNode;
	
	//construct monitors and associate them to our nodes
	PubMonitor innerMonitor(innerNode, outerNode, BridgeType::LOCAL);
	PubMonitor outerMonitor(outerNode, innerNode, BridgeType::REMOTE);
	innerNode.addPublisherMonitor(&innerMonitor);
	outerNode.addPublisherMonitor(&outerMonitor);
	
	//construct discovery and add inner node to discovery
	MDNSDiscoveryOptions mdnsOpts;
	if(domain)
		mdnsOpts.setDomain(domain);
	Discovery disc(Discovery::MDNS, &mdnsOpts);
	disc.add(innerNode);
	
	//initialize bridge internal ping subscriber and publisher
	PingReceiver* pingRecv = new PingReceiver();
	Subscriber subPing("umundo-bridge.ping", pingRecv);
	Publisher pubPing("umundo-bridge.ping");
	outerNode.addSubscriber(subPing);
	outerNode.addPublisher(pubPing);
	
	if(verbose)
	{
		std::cout << "Created bridge internal ping publisher " << pubPing << std::endl;
		std::cout << "Created bridge internal ping subscriber " << subPing << std::endl;
	}
	
	//mainloop (internal ping and bridge (re)connect handling)
	while(true) {
		//connect outer node to remote bridge endpoint
		if(verbose)
			std::cout << "Connecting to remote bridge instance at " << endPoint << std::endl;
		outerNode.added(endPoint);
		for(int i=0; i<60*4 && !outerNode.connectedTo().size(); i++)
			Thread::sleepMs(1000);		//wait for connection to be established
		if(!outerNode.connectedTo().size())
		{
			if(verbose)
				std::cout << "Connection failed, trying again in 60 seconds!" << std::endl;
			Thread::sleepMs(60000);
			continue;
		}
		//wait for disconnect and ping remote bridge endpoint
		while(outerNode.connectedTo().size()) {
			Message* msg = new Message();
			msg->setData("ping", 4);
			if(verbose)
				std::cout << "o" << std::flush;
			pubPing.send(msg);
			delete(msg);
			Thread::sleepMs(5000);
		}
		if(verbose)
			std::cout << "Connection to remote bridge instance at " << endPoint << " lost, trying to reconnect" << std::endl;
	}

}
