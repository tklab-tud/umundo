/**
 *  Copyright (C) 2014  Thilo Molitor (thilo@eightysoft.de)
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
#include <exception>
#include <queue>

#ifdef WIN32
#include "XGetopt.h"
#include <Winsock2.h>
#include <Iphlpapi.h>
#endif

#ifdef UNIX
#include <sys/socket.h>
#include <unistd.h> // gethostname
#if !defined(ANDROID)
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#endif

#include <string.h> // strerror
#include <errno.h> // errno
#include <sstream>
#include <iomanip>

#ifdef APPLE
#include <net/if_dl.h> // sockaddr_dl and LLADDR
#endif

#if defined(UNIX) && !defined(APPLE)
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#ifndef SOCKET
	#define SOCKET int
#endif

using namespace umundo;

char* domain = NULL;
bool verbose = false;

void printUsageAndExit() {
	printf("umundo-bridge version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage:\n");
	printf("\tumundo-bridge [-d domain] [-v] -c IPv4:port\n");
	printf("\tumundo-bridge [-d domain] [-v] -l port\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-d <domain>        : join domain\n");
	printf("\t-v                 : be more verbose\n");
	printf("\t-l <port>          : listen on this udp and tcp port\n");
	printf("\t-c <IPv4>:<port>   : connect to remote end at IPv4:port (via tcp and udp)\n");
	printf("\n");
	printf("Examples:\n");
	printf("\tumundo-bridge -l 4242\n");
	printf("\tumundo-bridge -c 130.32.14.22:4242\n");
	exit(1);
}

std::string ipToStr(uint32_t ip) {
	char* ip_p=(char*)&ip;
	std::string output=toStr((int)ip_p[0])+"."+toStr((int)ip_p[1])+"."+toStr((int)ip_p[2])+"."+toStr((int)ip_p[3]);
	return output;
}

class SocketException: public std::runtime_error {
public:
	explicit SocketException(const std::string& what) : std::runtime_error(what+": "+strerror(errno)) { }
	virtual ~SocketException() throw () { }
};

class BridgeMessageException: public std::runtime_error {
public:
	explicit BridgeMessageException(const std::string& what) : std::runtime_error("BridgeMessageException: "+what) { }
	virtual ~BridgeMessageException() throw () { }
};

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

/*
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
*/

//monitor addition and removal of new publishers and publish the same channel on the other side
class PubMonitor : public ResultSet<PublisherStub> {
private:
	Node& _receiverNode;
	std::map<std::string, Publisher*> _knownPubs;
	BridgeType _type;
	RMutex _mutex;
	
public:
	PubMonitor(Node& receiverNode, BridgeType type) : _receiverNode(receiverNode), _type(type) { }
	
	void added(PublisherStub pubStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got new " << _type << " publisher: " << pubStub << " --> publishing on " << !_type << " side..." << std::endl;
		/*
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
		*/
	}
	
	void removed(PublisherStub pubStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got removal of " << _type << " publisher: " << pubStub << " --> also removing on " << !_type << " side..." << std::endl;
		/*
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
		*/
	}
	
	void changed(PublisherStub pubStub) {
		//do nothing here
	}
};

//for 64bit message entry lengths see https://stackoverflow.com/questions/809902/64-bit-ntohl-in-c
class BridgeMessage {
private:
	uint16_t _protocolVersion;
	std::map<std::string, std::string> _data;
	typedef std::pair<const char*, size_t> MessageBuffer;
	
	void advanceBuffer(MessageBuffer& buffer, size_t length) {
		if(buffer.second < length)
			throw BridgeMessageException("Message data truncated");
		buffer.first+=length;
		buffer.second-=length;
	}
	
	template<typename T>
	const T readFromBuffer(MessageBuffer& buffer) {
		const char* value=buffer.first;
		advanceBuffer(buffer, sizeof(T));
		if(sizeof(T)==sizeof(uint32_t))
			return ntohl(*(const T*)value);			//uint32_t network byte order (bigEndian) to host byte order
		else if(sizeof(T)==sizeof(uint16_t))
			return ntohs(*(const T*)value);			//uint16_t network byte order (bigEndian) to host byte order
		else
			return *(const T*)value;					//warning: no implicit endianes conversions done for this (unknown) types
	}
	
	const std::string readFromBuffer(MessageBuffer& buffer, size_t length) {
		const char* value=buffer.first;
		advanceBuffer(buffer, length);
		return std::string(value, length);
	}

public:
	//construct empty message
	BridgeMessage() : _protocolVersion(1) { }
	
	//unserialize message
	BridgeMessage(const char* msg, size_t msgsize) {
		MessageBuffer buffer(msg, msgsize);
		uint16_t version = readFromBuffer<uint16_t>(buffer);
		switch(version) {
			case 1:
				_protocolVersion=version;
				break;
			default:
				throw BridgeMessageException("Unknown protocol version ("+toStr(version)+")");
		}
		do {
			uint32_t keySize = readFromBuffer<uint32_t>(buffer);
			if(!keySize)
				break;			//message end reached
			std::string key = readFromBuffer(buffer, keySize);
			uint32_t valueSize = readFromBuffer<uint32_t>(buffer);
			std::string value = readFromBuffer(buffer, valueSize);
			_data[key]=value;
		} while(true);
		if(buffer.second>0)
			throw BridgeMessageException("Unexpected message padding found ("+toStr(buffer.second)+" bytes)");
	}
	
	BridgeMessage(const BridgeMessage& other) : _protocolVersion(other._protocolVersion) {
		//STL containers will copy themselves
		_data = other._data;
	}
	
	//serialize message
	std::string toString() {
		uint16_t version = htons(_protocolVersion);
		std::string output = std::string((char*)&version, sizeof(version));
		//iterate over our key-value-pairs
		uint32_t keyLength, valueLength;
		for(std::map<std::string, std::string>::iterator it=_data.begin(); it!=_data.end(); ++it) {
			keyLength=htonl(it->first.length());
			output+=std::string((char*)&keyLength, sizeof(uint32_t));
			output+=it->first;
			
			valueLength=htonl(it->second.length());
			output+=std::string((char*)&valueLength, sizeof(uint32_t));
			output+=it->second;
		}
		//mark message end (zero sized key)
		keyLength=0;
		output+=std::string((char*)&keyLength, sizeof(uint32_t));
		return output;
	}
	
	template<typename T>
	void set(const std::string& key, const T& value) {
		_data[key]=toStr(value);
	}
	
	const std::map<std::string, std::string>& get() {
		return _data;
	}
	
	const std::string get(const std::string& key) {
		if(isset(key))
			return _data[key];
		return "";
	}
	
	template<typename T>
	const T get(const std::string& key) {
		return strTo<T>(get(key));
	}
	
	bool isset(const std::string& key) {
		return _data.find(key) != _data.end();
	}
};

class MessageQueue {
private:
	std::queue<BridgeMessage*> _queue;
	RMutex _mutex;
	Monitor _cond;
	
	MessageQueue() { }
	
	void write(BridgeMessage* msg) {
		RScopeLock lock(_mutex);
		_queue.push(msg);
		_cond.broadcast();
	}
	
	BridgeMessage* read() {
		return read(0);
	}
	
	BridgeMessage* read(uint32_t timeout_ms) {
		RScopeLock lock(_mutex);
		if(_queue.empty())
			_cond.wait(_mutex, timeout_ms);
		if(_queue.empty())
			return NULL;
		BridgeMessage* msg=_queue.front();
		_queue.pop();
		return msg;
	}
	
	bool empty(void) {
		RScopeLock lock(_mutex);
		return _queue.empty();
	}
	
	friend class TCPListener;
	friend class UDPListener;
	friend class Connector;
};

class UDPListener : public Thread {
private:
	SOCKET _socket;
	MessageQueue _queue;
	
	UDPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue) {
		start();
	}
	
	void run() {
		char buffer[65536]="";
		while(true) {
			size_t count=recv(_socket, buffer, sizeof(buffer), 0);
			if(count==-1)
				throw SocketException("recv() on udp socket failed");
			else {
				BridgeMessage* msg;
				try {
					msg=new BridgeMessage(buffer, count);
				} catch(BridgeMessageException& e) {
					delete msg;
					throw e;		///TODO: cancel connection and try to connect/listen again
				}
				_queue.write(msg);
			}
		}
	}
	
	friend class Connector;
};

class TCPListener : public Thread {
private:
	SOCKET _socket;
	MessageQueue _queue;
	
	TCPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue) {
		start();
	}
	
	void run() {
		char buffer[65536]="";
		while(true) {
			///TODO: concat incoming data till one complete message is received (eg. packetize incoming stream)
			size_t count=recv(_socket, buffer, sizeof(buffer), 0);
			if(count==-1)
				throw SocketException("recv() on tcp socket failed");
			else {
				BridgeMessage* msg;
				try {
					msg=new BridgeMessage(buffer, count);
				} catch(BridgeMessageException& e) {
					delete msg;
					throw e;		///TODO: cancel connection and try to connect/listen again
				}
				_queue.write(msg);
			}
		}
	}
	
	friend class Connector;
};

class Connector : public Thread {
private:
	SOCKET _tcpSocket;
	SOCKET _udpSocket;
	MessageQueue _messageQueue;
	TCPListener* _tcpListener;
	UDPListener* _udpListener;
	bool _listening;
	bool _handleRTP;

public:
	enum dummy { UDP, TCP };
	
	Connector(uint16_t listenPort) : _listening(true), _handleRTP(false) {
		initNetwork();
		
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		addr.sin_port = htons(listenPort);
		
		//see http://www.microhowto.info/howto/listen_on_a_tcp_port_with_connections_in_the_time_wait_state.html#idp25744
		int reuseaddr = 1;
		if(setsockopt(_tcpSocket, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr))==-1)
			throw SocketException("Could not set SO_REUSEADDR on tcp socket");
		if(bind(_tcpSocket,(struct sockaddr*)&addr, sizeof(addr)) < 0)
			throw SocketException("Could not bind tcp socket to specified port: ");
		if(bind(_udpSocket,(struct sockaddr*)&addr, sizeof(addr)) < 0)
			throw SocketException("Could not bind udp socket to specified port");
		if(listen(_tcpSocket, SOMAXCONN)==-1)
			throw SocketException("Could not listen on tcp socket");
	}
	
	Connector(std::string connectIP, uint16_t connectPort) : _listening(false) {
		initNetwork();
		
		
	}
	
	~Connector() {
		delete _udpListener;
		#ifdef WIN32
			closesocket(_tcpSocket);
			closesocket(_udpSocket);
			WSACleanup();
		#else
			close(_tcpSocket);
			close(_udpSocket);
		#endif
	}
	
	void waitForConnection() {
		if(_listening) {
			SOCKET newSocket;
			struct sockaddr_in clientAddress;
			size_t clientAddressLength=sizeof(clientAddress);
			while(true) {
				newSocket = accept(_tcpSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
				if(newSocket == -1) {
					if(errno == EINTR)
						continue;
					throw SocketException("Could not accept connection on tcp socket");
				}
				break;
			}
			#ifdef WIN32
				closesocket(_tcpSocket);
			#else
				close(_tcpSocket);
			#endif
			_tcpSocket = newSocket;
			
			if(verbose)
				std::cout << "Accepted connection from " << ipToStr(clientAddress.sin_addr.s_addr) << ":" << ntohs(clientAddress.sin_port) << " on tcp socket, sending serverHello message..." << std::endl;
			
			if(connect(_udpSocket, (sockaddr*)&clientAddress, sizeof(clientAddress))==-1)
				throw SocketException("Could not connect udp socket to remote ip");
			_tcpListener=new TCPListener(_tcpSocket, _messageQueue);
			_udpListener=new UDPListener(_udpSocket, _messageQueue);
			
			BridgeMessage* msg=new BridgeMessage();
			msg->set("type", "serverHello");
			sendMessage(*msg, TCP);
			delete msg;
			msg=NULL;
			for(int timeout=0; timeout<8 && msg==NULL; timeout++)	///8 seconds timeout for udp socket confirmation (clientHello)
				msg=_messageQueue.read(1000);
			if(msg==NULL) {
				std::cout << "WARNING: udp connection not functional --> only forwarding non-rtp pubs/subs..." << std::endl;
				msg=new BridgeMessage();
				msg->set("type", "alert");
				msg->set("alert", "udpDysfunction");
				sendMessage(*msg, TCP);
				delete msg;
			} else {
				_handleRTP=true;
				if(msg->get("type")!="clientHello")
					std::cout << "WARNING: packet loss on udp connection --> using it anyway..." << std::endl;
			}
		} else {
			///TODO: connect tcp and udp sockets
		}
		start();
		
	}
	
	void waitForDisconnection() {
		///TODO: do something useful
	}
	
	void sendMessage(BridgeMessage& msg, dummy channel) {
		std::string packet;
		if(channel==TCP) {
			//prefix message with message length so that our tcp stream could be split into individual messages easyliy at the remote bridge instance
			std::string message=msg.toString();
			uint32_t messageSize=htonl(message.length());
			packet = std::string((char*)&messageSize, sizeof(uint32_t));
			packet+=message;
		}
		else
			packet=msg.toString();
		if(send(channel==TCP ? _tcpSocket : _udpSocket, packet.c_str(), packet.length(), 0)!=packet.length())
			throw SocketException(std::string("Could not send data on ")+(channel==TCP ? "tcp" : "udp")+" socket");
	}

private:
	void run() {
		while(true) {
			///TODO: read messages from the queue and process them, also alert main thread (waiting in waitForDisconnection()) when disconnected
			Thread::sleepMs(1000);
		}
	}
	
	inline void initNetwork() {
		//initialize networking
		#ifdef WIN32
			WSADATA wsaData;
			WSAStartup(MAKEWORD(2, 2), &wsaData);
		#endif
		_tcpSocket = socket(PF_INET, SOCK_STREAM, 0);
		_udpSocket = socket(PF_INET, SOCK_DGRAM, 0);
	}
};

int main(int argc, char** argv) {
	int option;
	int listen = 0;
	std::string remoteIP = "";
	uint16_t remotePort = 0;
	Connector* connector;
	
	while((option = getopt(argc, argv, "vd:l:c:")) != -1) {
		switch(option) {
		case 'd':
			domain = optarg;
			break;
		case 'v':
			verbose = true;
			break;
		case 'l':
			listen = strTo<uint16_t>(optarg);
			if(listen == 0)
				printUsageAndExit();
			break;
		case 'c': {
			EndPoint endPoint(std::string("tcp://")+std::string(optarg));
			if(!endPoint)
				printUsageAndExit();
			remoteIP = endPoint.getIP();
			remotePort = endPoint.getPort();
			break;
		}
		default:
			printUsageAndExit();
			break;
		}
	}
	if(optind < argc || (listen == 0 && remotePort == 0))
		printUsageAndExit();
	
	if(listen) {
		if(verbose)
			std::cout << "Listening at local port " << listen << std::endl;
		connector = new Connector(listen);
		connector->waitForConnection();
	} else {
		if(verbose)
			std::cout << "Connecting to remote bridge instance at " << remoteIP << ":" << remotePort << std::endl;
		connector = new Connector(remoteIP, remotePort);
		connector->waitForConnection();
	}
	
	return 0;
	
	//construct inner node
	Node innerNode;
	
	//construct monitor and associate it to our node
	PubMonitor innerMonitor(innerNode, BridgeType::LOCAL);
	innerNode.addPublisherMonitor(&innerMonitor);
	
	//construct discovery and add inner node to discovery
	MDNSDiscoveryOptions mdnsOpts;
	if(domain)
		mdnsOpts.setDomain(domain);
	Discovery disc(Discovery::MDNS, &mdnsOpts);
	disc.add(innerNode);
	
	//mainloop
	while(true) {
		connector->waitForDisconnection();
		Thread::sleepMs(1000);
	}
}
