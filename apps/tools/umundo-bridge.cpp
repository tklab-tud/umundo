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
#include <boost/shared_ptr.hpp>

#ifdef WIN32
#include "XGetopt.h"
#include <Winsock2.h>
#include <Iphlpapi.h>
#endif

#ifdef UNIX
#include <sys/socket.h>
#include <unistd.h> // gethostname
#include <netdb.h>
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

#define TIMEOUT 8

using namespace umundo;

char* domain = NULL;
bool verbose = false;

//some helper functions
void printUsageAndExit() {
	printf("umundo-bridge version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage:\n");
	printf("\tumundo-bridge [-d domain] [-v] -c <hostnameOrIPv4>:<port>\n");
	printf("\tumundo-bridge [-d domain] [-v] -l port\n");
	printf("\n");
	printf("Options:\n");
	printf("\t-d <domain>                  : join domain\n");
	printf("\t-v                           : be more verbose\n");
	printf("\t-l <port>                    : listen on this udp and tcp port\n");
	printf("\t-c <hostnameOrIPv4>:<port>   : connect to remote end at IPv4:port (via tcp and udp)\n");
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

//some exceptions
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

//stream output of publisher or subscriber
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

class ProtocolHandler;

//send incoming umundo messages through our bridge
class GlobalReceiver : public Receiver {
private:
	std::string _channelName;
	boost::shared_ptr<ProtocolHandler> _handler;

public:
	GlobalReceiver(std::string, boost::shared_ptr<ProtocolHandler>);
	void receive(Message*);
};

//monitor subscriptions to our publishers and subscribe on the other side if neccessary
class GlobalGreeter : public Greeter {
private:
	Publisher* _pub;
	boost::shared_ptr<ProtocolHandler> _handler;
	
public:
	GlobalGreeter(Publisher*, boost::shared_ptr<ProtocolHandler>);
	void welcome(Publisher&, const SubscriberStub&);
	void farewell(Publisher&, const SubscriberStub&);
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
			return *(const T*)value;				//warning: no implicit endianes conversions done for this (unknown) types
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
	
	std::string& operator[](std::string& key) {
		return _data[key];
	}
	
	std::string& operator[](std::string key) {
		return _data[key];
	}
	
	std::string& operator[](const char* key) {
		return _data[std::string(key)];
	}
};

class MessageQueue {
private:
	std::queue<BridgeMessage*> _queue;
	RMutex _mutex;
	Monitor _cond;
	
	MessageQueue() { }
	
	MessageQueue(MessageQueue& other) : _queue(other._queue), _mutex(other._mutex), _cond(other._cond) { }
	
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
		if(_queue.empty())		//timeout occured --> return NULL pointer
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
	friend class ProtocolHandler;
};

class TCPListener : public Thread {
private:
	SOCKET _socket;
	MessageQueue& _queue;
	
	TCPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue) {
		start();
	}
	
	void terminate() {
		///TODO: terminate thread
	}
	
	void run() {
		try {
			char* buffer;
			uint32_t packetSize;
			while(true) {
				packetSize=htonl(0);			//stays zero after recvAll() if connection was closed
				if(recvAll(_socket, (char*)&packetSize, sizeof(uint32_t), 0)<0)
					throw SocketException("recv() of packetSize on tcp socket failed");
				packetSize=ntohl(packetSize);
				if(packetSize==0)				//socket read returned 0 --> connection was closed
					break;
				buffer=(char*)malloc(packetSize);
				if(buffer==NULL)
					throw std::runtime_error("malloc for "+toStr(packetSize)+" bytes failed");
				if(recvAll(_socket, buffer, packetSize, 0)<=0) {
					free(buffer);
					throw SocketException("recv() of packet on tcp socket failed");
				}
				else {
					BridgeMessage* msg;
					try {
						msg=new BridgeMessage(buffer, packetSize);
					} catch(std::exception& e) {
						free(buffer);
						delete msg;
						throw e;
					}
					_queue.write(msg);
					free(buffer);
					delete msg;
				}
			}
		} catch(std::exception& e) {
			if(verbose)
				std::cout << "Received exception in TCPListener: " << e.what() << std::endl;
		} catch(...) {
			if(verbose)
				std::cout << "Received unknown exception in TCPListener"<< std::endl;
		}
		if(verbose)
			std::cout << "Terminating TCPListener..."<< std::endl;
		BridgeMessage* terminationMessage = new BridgeMessage();
		terminationMessage->set("type", "internal");
		terminationMessage->set("source", "TCPListener");
		terminationMessage->set("cause", "termination");
		_queue.write(terminationMessage);
	}
	
	int recvAll(SOCKET socket, char* buffer, size_t length, int flags) {
		int retval;
		size_t originalLength=length;
		do {
			retval=recv(socket, buffer, length, 0);
			if(retval<=0)
				return retval;
			length-=retval;		//calculate remaining data length
			buffer+=retval;		//advance buffer
		} while(length>0);
		return originalLength;
	}
	
	friend class ProtocolHandler;
};

class UDPListener : public Thread {
private:
	SOCKET _socket;
	MessageQueue& _queue;
	
	UDPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue) {
		start();
	}
	
	void terminate() {
		///TODO: terminate thread
	}
	
	void run() {
		char buffer[65536]="";
		try {
			while(true) {
				size_t count=recv(_socket, buffer, sizeof(buffer), 0);
				if(count==-1)
					throw SocketException("recv() on udp socket failed");
				else {
					BridgeMessage* msg;
					try {
						msg=new BridgeMessage(buffer, count);
					} catch(std::exception& e) {
						delete msg;
						throw e;
					}
					_queue.write(msg);
					delete msg;
				}
			}
		} catch(std::exception& e) {
			if(verbose)
				std::cout << "Received exception in UDPListener: " << e.what() << std::endl;
		} catch(...) {
			if(verbose)
				std::cout << "Received unknown exception in UDPListener"<< std::endl;
		}
		if(verbose)
			std::cout << "Terminating UDPListener..."<< std::endl;
		BridgeMessage* terminationMessage = new BridgeMessage();
		terminationMessage->set("type", "internal");
		terminationMessage->set("source", "UDPListener");
		terminationMessage->set("cause", "termination");
		_queue.write(terminationMessage);
	}
	
	friend class ProtocolHandler;
};

class ProtocolHandler : public boost::enable_shared_from_this<ProtocolHandler> {
private:
	SOCKET _tcpSocket;
	SOCKET _udpSocket;
	bool _handleRTP;
	bool _shutdownDone;
	MessageQueue _queue;
	TCPListener* _tcpListener;
	UDPListener* _udpListener;
	RMutex _shutdownMutex;
	std::map<std::string, Publisher*> _knownPubs;
	std::map<std::string, std::pair<Subscriber*, std::map<std::string, Subscriber*> > > _knownSubs;
	enum dummy { UDP, TCP };

public:
	ProtocolHandler(SOCKET tcpSocket, SOCKET udpSocket, bool handleRTP) : _tcpSocket(tcpSocket), _udpSocket(udpSocket), _handleRTP(handleRTP), _shutdownDone(false) {
		_tcpListener=new TCPListener(_tcpSocket, _queue);
		if(_handleRTP)
			_udpListener=new UDPListener(_udpSocket, _queue);
	}
	
	~ProtocolHandler() {
		shutdown(NULL);
	}
	
	void send_pubAdded(std::string channelName) {
		BridgeMessage msg;
		msg["type"]="pubAdded";
		msg["channelName"]=channelName;
		sendMessage(msg, TCP);
	}
	
	void send_pubRemoved(std::string channelName) {
		BridgeMessage msg;
		msg["type"]="pubRemoved";
		msg["channelName"]=channelName;
		sendMessage(msg, TCP);
	}
	
	void send_subAdded(std::string channelName) {
		BridgeMessage msg;
		msg["type"]="subAdded";
		msg["channelName"]=channelName;
		sendMessage(msg, TCP);
	}
	
	void send_subRemoved(std::string channelName) {
		BridgeMessage msg;
		msg["type"]="subRemoved";
		msg["channelName"]=channelName;
		sendMessage(msg, TCP);
	}
	
	void send_data(std::string channelName, Message* umundoMessage) {
		//fetch data from all "silent" subscribers and discard it
		std::map<std::string, Subscriber*>::iterator subsIter=_knownSubs[channelName].second.begin();
		while(subsIter!=_knownSubs[channelName].second.end()) {
			while(subsIter->second->hasNextMsg())
				subsIter->second->getNextMsg();
			subsIter++;
		}
		//"serialize" and send umundoMessage
		BridgeMessage msg;
		msg["type"]="data";
		msg["channelName"]=channelName;
		msg["data"]=std::string(umundoMessage->data(), umundoMessage->size());
		uint32_t metadataCount=0;
		std::map<std::string, std::string>::const_iterator metaIter=umundoMessage->getMeta().begin();
		while(metaIter!=umundoMessage->getMeta().end()) {
			msg["metadataKey."+toStr(metadataCount)]=metaIter->first;
			msg["metadataValue."+toStr(metadataCount)]=metaIter->second;
			metadataCount++;
			metaIter++;
		}
		msg["metadataCount"]=toStr(metadataCount);
		sendMessage(msg, TCP);
	}
	
	//read messages from the queue and process them
	//no locking needed (single threaded)
	void mainloop(Node& innerNode) {
		while(true) {
			///TODO: dont forget to ping via udp when only tcp is active
			BridgeMessage* msg_p=_queue.read(15000);		//ping every 15 seconds when idle
			if(msg_p==NULL) {		//use read timeout to periodically send tcp and udp pings (to keep nat open etc.)
				BridgeMessage msg;
				msg["type"]="internal";
				msg["cause"]="ping";
				sendMessage(msg, UDP);
				sendMessage(msg, TCP);
				continue;
			}
			BridgeMessage& msg=*msg_p;
			if(msg["type"]=="internal") {
				if(msg["cause"]=="termination")
					break;
				if(msg["cause"]=="ping")
					continue;
			}
			if(verbose)
				std::cout << "Got message of type '" << msg["type"] << "'..." << std::endl;
			
			if(msg["type"]=="pubAdded") {
				///TODO: index pubs using UUID (see PubMonitor todo)
				Publisher* ownPub = new Publisher(msg["channelName"]);
				GlobalGreeter* ownGreeter = new GlobalGreeter(ownPub, shared_from_this());
				ownPub->setGreeter(ownGreeter);
				_knownPubs[msg["channelName"]] = ownPub;
				innerNode.addPublisher(*ownPub);
				if(verbose)
					std::cout << "Created new LOCAL publisher " << ownPub->getUUID() << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
			
			} else if(msg["type"]=="pubRemoved") {
				std::string localUUID = _knownPubs[msg["channelName"]]->getUUID();
				GlobalGreeter* greeter = dynamic_cast<GlobalGreeter*>(_knownPubs[msg["channelName"]]->getGreeter());
				_knownPubs[msg["channelName"]]->setGreeter(NULL);
				
				//unsubscribe all subscribers for this channel
				if(_knownSubs[msg["channelName"]].first!=NULL)
					for(uint32_t c=0; c<_knownSubs[msg["channelName"]].second.size()+1; c++)
						unsubscribe(msg["channelName"], innerNode);
				
				innerNode.removePublisher(*_knownPubs[msg["channelName"]]);
				_knownPubs.erase(msg["channelName"]);
				delete _knownPubs[msg["channelName"]];
				delete greeter;
				if(verbose)
					std::cout << "Removed LOCAL publisher " << localUUID << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
			
			} else if(msg["type"]=="subAdded") {
				Subscriber* sub;
				if(_knownSubs[msg["channelName"]].first==NULL) {
					GlobalReceiver* receiver = new GlobalReceiver(msg["channelName"], shared_from_this());
					sub = new Subscriber(msg["channelName"], receiver);
					_knownSubs[msg["channelName"]].first = sub;
				} else {			//first subscriber --> register actual receiver to receive the data
					sub = new Subscriber(msg["channelName"]);
					_knownSubs[msg["channelName"]].second[sub->getUUID()] = sub;
				}
				innerNode.addSubscriber(*sub);
				if(verbose)
					std::cout << "Created new LOCAL subscriber " << sub->getUUID() << " for REMOTE subscriber on channel '" << msg["channelName"] << "'" << std::endl;
			
			} else if(msg["type"]=="subRemoved") {
				std::string localUUID;
				unsubscribe(msg["channelName"], innerNode);
				if(verbose && localUUID.length())
					std::cout << "Removed LOCAL subscriber " << localUUID << " for REMOTE subscriber on channel '" << msg["channelName"] << "'" << std::endl;
			
			} else if(msg["type"]=="data") {
				Message* umundoMessage = new Message(msg["data"].c_str(), msg["data"].length());
				uint32_t metadataCount = msg.get<uint32_t>("metadataCount");
				for(uint32_t c=0; c<metadataCount; c++)
					umundoMessage->putMeta(msg["metadataKey."+toStr(c)], msg["metadataValue."+toStr(c)]);
				_knownPubs[msg["channelName"]]->send(umundoMessage);
				delete(umundoMessage);
			
			} else {
				std::cout << "WARNING: unknown message type '" << msg["type"] << "', protocol mismatch? --> closing connection..." << std::endl;
				break;
			}
			delete msg_p;
		}
		if(verbose)
			std::cout << "ProtocolHandler lost connection..."<< std::endl;
		shutdown(&innerNode);
	}

private:
	std::string unsubscribe(std::string channelName, Node& innerNode) {
		std::string localUUID;
		if(_knownSubs[channelName].second.empty()) {		//last subscriber removed --> remove our master subscriber
			if(_knownSubs[channelName].first==NULL) {
				if(verbose)
					std::cout << "REMOTE subscriber for channel '" << channelName << "' unknown, ignoring removal!" << std::endl;
			} else {
				Subscriber* sub=_knownSubs[channelName].first;
				localUUID=sub->getUUID();
				Receiver* receiver = sub->getReceiver();
				sub->setReceiver(NULL);
				innerNode.removeSubscriber(*sub);
				_knownSubs[channelName].first=NULL;
				delete sub;
				delete receiver;
			}
		} else {			//this was not our last subscriber --> remove only "silent" subscriber
			Subscriber* sub=_knownSubs[channelName].second.begin()->second;
			localUUID=sub->getUUID();
			innerNode.removeSubscriber(*sub);
			_knownSubs[channelName].second.erase(sub->getUUID());
			delete sub;
		}
		return localUUID;
	}
	
	void shutdown(Node* innerNode) {
		RScopeLock lock(_shutdownMutex);
		if(_shutdownDone)
			return;
		_shutdownDone=true;
		
		if(verbose)
			std::cout << "Shutting down ProtocolHandler..." << std::endl;
		
		//remove all publishers
		std::map<std::string, Publisher*>::iterator pubIter = _knownPubs.begin();
		while(pubIter != _knownPubs.end())
		{
			if(verbose)
				std::cout << "Removing LOCAL publisher " << *pubIter->second << std::endl;
			GlobalGreeter* greeter = dynamic_cast<GlobalGreeter*>(pubIter->second->getGreeter());
			pubIter->second->setGreeter(NULL);
			
			//unsubscribe all subscribers for this channel
			if(_knownSubs[pubIter->second->getChannelName()].first!=NULL)
				for(uint32_t c=0; c<_knownSubs[pubIter->second->getChannelName()].second.size()+1; c++)
					unsubscribe(pubIter->second->getChannelName(), *innerNode);
			
			if(innerNode!=NULL)
				innerNode->removePublisher(*pubIter->second);
			delete pubIter->second;
			delete greeter;
			pubIter++;
		}
		_knownPubs.clear();
		
		//terminate all socket listeners
		_tcpListener->terminate();
		delete _tcpListener;
		if(_handleRTP) {
			_udpListener->terminate();
			delete _udpListener;
		}
		
		//close all sockets
		#ifdef WIN32
			closesocket(_tcpSocket);
			closesocket(_udpSocket);
			WSACleanup();
		#else
			close(_tcpSocket);
			close(_udpSocket);
		#endif
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
};

GlobalReceiver::GlobalReceiver(std::string channelName, boost::shared_ptr<ProtocolHandler> handler) : _channelName(channelName), _handler(handler) { }

void GlobalReceiver::receive(Message* msg) {
	//if(verbose)
	//	std::cout << "Forwarding message..." << std::endl;
	_handler->send_data(_channelName, msg);
}

GlobalGreeter::GlobalGreeter(Publisher* pub, boost::shared_ptr<ProtocolHandler> handler) : _pub(pub), _handler(handler) { }

void GlobalGreeter::welcome(Publisher& pub, const SubscriberStub& subStub) {
	if(verbose)
		std::cout << "Got new LOCAL subscriber: " << subStub << " to publisher " << *_pub << std::endl;
	if(verbose)
		std::cout << "--> subscribing on REMOTE side..." << std::endl;
	_handler->send_subAdded(_pub->getChannelName());
}

void GlobalGreeter::farewell(Publisher& pub, const SubscriberStub& subStub) {
	if(verbose)
		std::cout << "Removed LOCAL subscriber: " << subStub << " to publisher " << *_pub << std::endl;
	if(verbose)
		std::cout << "--> unsubscribing on REMOTE side..." << std::endl;
	_handler->send_subRemoved(pub.getChannelName());
}

//monitor addition and removal of new publishers and publish the same channel on the other side
class PubMonitor : public ResultSet<PublisherStub> {
private:
	Node& _innerNode;
	std::map<std::string, uint32_t> _knownPubs;
	boost::shared_ptr<ProtocolHandler> _handler;
	RMutex _mutex;
	
public:
	PubMonitor(Node& innerNode, boost::shared_ptr<ProtocolHandler> handler) : _innerNode(innerNode), _handler(handler) { }
	
	///TODO: use UUID instead of channelName to distinguish publishers (>1 publishers on one side should result in >1 publisher on the other side)
	void added(PublisherStub pubStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got new LOCAL publisher: " << pubStub << std::endl;
		if(_knownPubs.find(pubStub.getChannelName())!=_knownPubs.end())
		{
			if(verbose)
				std::cout << "--> LOCAL publisher for channel '" << pubStub.getChannelName() << "' already known, ignoring!" << std::endl;
			_knownPubs[pubStub.getChannelName()]++;
			return;
		}
		_knownPubs[pubStub.getChannelName()]=1;
		if(verbose)
			std::cout << "--> sending PUB_ADDED for channel '" << pubStub.getChannelName() << "' through bridge..." << std::endl;
		_handler->send_pubAdded(pubStub.getChannelName());
	}
	
	void removed(PublisherStub pubStub) {
		RScopeLock lock(_mutex);
		if(verbose)
			std::cout << "Got removal of LOCAL publisher: " << pubStub << std::endl;
		if(_knownPubs.find(pubStub.getChannelName())==_knownPubs.end() || _knownPubs[pubStub.getChannelName()]==0) {
			if(verbose)
				std::cout << "--> LOCAL publisher for channel '" << pubStub.getChannelName() << "' unknown, ignoring removal!" << std::endl;
			return;
		}
		_knownPubs[pubStub.getChannelName()]--;
		if(_knownPubs[pubStub.getChannelName()]==0) {
			_knownPubs.erase(pubStub.getChannelName());
			if(verbose)
				std::cout << "--> was last publisher, sending PUB_REMOVED for channel '" << pubStub.getChannelName() << "' through bridge..." << std::endl;
			_handler->send_pubRemoved(pubStub.getChannelName());
		}
	}
	
	void changed(PublisherStub pubStub) {
		//do nothing here
	}
};

class Connector {
private:
	SOCKET _tcpSocket;
	SOCKET _udpSocket;
	bool _listening;
	std::string _connectIP;
	uint16_t _connectPort;
	bool _handoffDone;
	enum dummy { UDP, TCP };

public:
	Connector(uint16_t listenPort) : _listening(true), _connectIP(""), _connectPort(0), _handoffDone(false) {
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
		if(listen(_tcpSocket, 1)==-1)
			throw SocketException("Could not listen on tcp socket");
	}
	
	Connector(std::string connectIP, uint16_t connectPort) : _listening(false), _connectIP(connectIP), _connectPort(connectPort) {
		initNetwork();
	}
	
	~Connector() {
		if(!_handoffDone) {			//dont close our sockets if handoff to ProtocolHandler is already done
			#ifdef WIN32
				closesocket(_tcpSocket);
				closesocket(_udpSocket);
				WSACleanup();
			#else
				close(_tcpSocket);
				close(_udpSocket);
			#endif
		}
	}
	
	boost::shared_ptr<ProtocolHandler> waitForConnection() {
		bool handleRTP=true;
		struct sockaddr_in remoteAddress;
		socklen_t remoteAddressLength=sizeof(remoteAddress);
		
		if(_listening) {
			SOCKET newSocket;
			while(true) {
				newSocket = accept(_tcpSocket, (struct sockaddr*)&remoteAddress, &remoteAddressLength);
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
				std::cout << "Accepted connection from " << ipToStr(remoteAddress.sin_addr.s_addr) << ":" << ntohs(remoteAddress.sin_port) << " on tcp socket, sending serverHello message..." << std::endl;
			
			//wait TIMEOUT seconds for clientHello on TCP channel
			if(waitForString("clientHello", TCP, TIMEOUT, NULL, 0))
				throw BridgeMessageException("Error while receiving initial 'clientHello' on tcp socket");
			//send serverHello on tcp channel
			if(send(_tcpSocket, "serverHello", 11, 0)!=11)
				throw SocketException("Could not send data on tcp socket");
			
			//wait TIMEOUT seconds for clientHello on UDP channel
			remoteAddressLength=sizeof(remoteAddress);
			if(waitForString("clientHello", UDP, TIMEOUT, (struct sockaddr*)&remoteAddress, &remoteAddressLength)) {
				handleRTP=false;
				std::cout << "WARNING: udp connection not functional --> only forwarding non-rtp pubs/subs..." << std::endl;
			} else {
				if(connect(_udpSocket, (sockaddr*)&remoteAddress, sizeof(remoteAddress))==-1)
					throw SocketException("Could not connect udp socket to remote ip");
			}
			//send serverHello on udp channel
			if(handleRTP) {
				if(send(_udpSocket, "serverHello", 11, 0)!=11)
					std::cout << "WARNING: Could not send data on udp socket" << std::endl;
			}
		} else {
			const char* hostname=_connectIP.c_str();
			struct addrinfo hints;
			memset(&hints, 0, sizeof(hints));
			hints.ai_family=AF_INET;
			hints.ai_socktype=SOCK_STREAM;
			hints.ai_protocol=0;
			hints.ai_flags=AI_ADDRCONFIG|AI_NUMERICSERV;
			struct addrinfo* res=0;
			int err=getaddrinfo(hostname, toStr(_connectPort).c_str(), &hints, &res);
			if(err!=0)
				throw SocketException("Failed to resolve hostname '"+_connectIP+"' (err="+toStr(err)+")");
			if(connect(_tcpSocket, res->ai_addr, res->ai_addrlen)==-1)		//use only the first returned address
				throw SocketException("Could not connect tcp socket");
			if(connect(_udpSocket, res->ai_addr, res->ai_addrlen)==-1)		//use only the first returned address
				throw SocketException("Could not connect udp socket");
			freeaddrinfo(res);
			
			//send "clientHello" on both channels
			if(send(_tcpSocket, "clientHello", 11, 0)!=11)
				throw SocketException("Could not send data on tcp socket");
			if(send(_udpSocket, "clientHello", 11, 0)!=11)
				handleRTP=false;
			
			//wait TIMEOUT seconds for resulting serverHello on TCP channel
			if(waitForString("serverHello", TCP, TIMEOUT, NULL, 0))
				throw BridgeMessageException("Error while receiving initial 'serverHello' on tcp socket");
			
			//wait TIMEOUT seconds for resulting serverHello on UDP channel
			if(handleRTP) {
				remoteAddressLength=sizeof(remoteAddress);
				try {
					if(waitForString("serverHello", UDP, TIMEOUT, (struct sockaddr*)&remoteAddress, &remoteAddressLength))
						throw std::exception();		//internal error reporting to surrounding try-catch block
				} catch(std::exception& e) {
					handleRTP=false;
				}
			}
			
			if(!handleRTP)
				std::cout << "WARNING: udp connection not functional --> only forwarding non-rtp pubs/subs..." << std::endl;
		}
		
		if(verbose)
				std::cout << "Connection successfully established, now handing off to ProtocolHandler..." << std::endl;
		
		_handoffDone=true;
		return boost::shared_ptr<ProtocolHandler>(new ProtocolHandler(_tcpSocket, _udpSocket, handleRTP));
	}

private:
	bool waitForString(std::string string, dummy type, uint32_t timeout_s, struct sockaddr* srcAddr, socklen_t* addrlen) {
		char* buffer;
		int retval;
		SOCKET socket;
		struct timeval time;
		#ifdef WIN32
			FD_SET receive;
		#else
			fd_set receive;
		#endif
		FD_ZERO(&receive);
		
		if(type==TCP)
			socket=_tcpSocket;
		else
			socket=_udpSocket;
		FD_SET(socket, &receive);
		
		time.tv_sec=timeout_s;
		time.tv_usec=0;
		retval=select(socket+1, &receive, NULL, NULL, &time);
		if(retval==EBADF || retval==EINTR || retval==EINVAL || retval==ENOMEM)
			throw SocketException("Could not use select() on sockets (err="+toStr(retval)+")");
		if(FD_ISSET(socket, &receive)) {
			buffer=(char*)malloc(string.length());
			if(type==TCP)
				retval=recvAll(socket, buffer, string.length(), 0, srcAddr, addrlen) ? 0 : string.length();
			else
				retval=recvfrom(socket, buffer, string.length(), 0, srcAddr, addrlen);
			if(retval>0 && retval!=string.length())
				throw std::runtime_error(std::string("Could not read on ")+(type==TCP ? "tcp" : "udp")+" socket ("+toStr(retval)+" != "+toStr(string.length())+")");
			else if(retval<=0)
				throw SocketException(std::string("Could not read on ")+(type==TCP ? "tcp" : "udp")+" socket ("+toStr(retval)+")");
			if(memcmp(buffer, string.c_str(), string.length())!=0)
				throw std::runtime_error(std::string("Corrupt init message received on ")+(type==TCP ? "tcp" : "udp")+" socket");
			free(buffer);
			return false;	//string received successfully
		}
		return true;		//timeout
	}
	
	bool recvAll(SOCKET socket, char* buffer, size_t length, int flags, struct sockaddr* srcAddr, socklen_t* addrlen) {
		int retval;
		do {
			retval=recvfrom(socket, buffer, length, 0, srcAddr, addrlen);
			if(retval<=0)
				return true;
			length-=retval;		//calculate remaining data length
			buffer+=retval;		//advance buffer
		} while(length>0);
		return false;
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
	boost::shared_ptr<ProtocolHandler> handler;
	
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
	
	while(true)
	{
		try {
			if(listen) {
				if(verbose)
					std::cout << "Listening at tcp and udp port " << listen << std::endl;
				connector = new Connector(listen);
				handler=connector->waitForConnection();
			} else {
				if(verbose)
					std::cout << "Connecting to remote bridge instance at " << remoteIP << ":" << remotePort << std::endl;
				connector = new Connector(remoteIP, remotePort);
				handler=connector->waitForConnection();
			}
			delete connector;
			
			//construct inner node
			Node innerNode;
			
			//construct monitor and associate it to our node
			PubMonitor innerMonitor(innerNode, handler);
			innerNode.addPublisherMonitor(&innerMonitor);
			
			//construct discovery and add inner node to discovery
			MDNSDiscoveryOptions mdnsOpts;
			if(domain)
				mdnsOpts.setDomain(domain);
			Discovery disc(Discovery::MDNS, &mdnsOpts);
			disc.add(innerNode);
			
			handler->mainloop(innerNode);
		} catch(std::runtime_error& e) {
			std::cout << "Got runtime_error with message '" << e.what() << "'" << std::endl;
		}
		std::cout << "Reconnecting in 5 seconds..." << std::endl;
		Thread::sleepMs(5000);
	}
}
