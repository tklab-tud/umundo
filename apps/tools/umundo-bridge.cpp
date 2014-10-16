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
#include <cassert>
#include <boost/shared_ptr.hpp>

#ifdef WIN32
#include "XGetopt.h"
#include <Winsock2.h>
#include <Iphlpapi.h>
#include <Ws2tcpip.h>
typedef int socklen_t;
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

//clientHello/serverHello timeout
#define TIMEOUT 8
//ping interval (to keep nat open etc.)
#define PING_INTERVAL 15

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
	printf("\t-t                           : run internal tests\n");
	printf("\t-d <domain>                  : join domain\n");
	printf("\t-v                           : be more verbose\n");
	printf("\t-l <port>                    : listen on this udp and tcp port\n");
	printf("\t-c <hostnameOrIPv4>:<port>   : connect to remote end at IPv4:port (via tcp and udp)\n");
	printf("\n");
	printf("Examples:\n");
	printf("\tumundo-bridge -l 4242\n");
	printf("\tumundo-bridge -c 130.32.14.22:4242\n");
	printf("\tumundo-bridge -t\n");
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
	bool _isRTP;
	boost::shared_ptr<ProtocolHandler> _handler;

public:
	GlobalReceiver(std::string, bool, boost::shared_ptr<ProtocolHandler>);
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
	template<typename T>
	struct identity { typedef T type; };		//dummy used by proxy template
	
	void advanceBuffer(MessageBuffer& buffer, size_t length) {
		if(buffer.second < length)
			throw BridgeMessageException("Message data truncated");
		buffer.first+=length;
		buffer.second-=length;
	}
	
	//proxy template to specialized template versions
	template<typename T>
	const T readFromBuffer(MessageBuffer& buffer) {
		return readFromBuffer(buffer, identity<T>());
	}
	
	template<typename T>
	const T readFromBuffer(MessageBuffer& buffer, identity<T> dummy) {
		const char* value=buffer.first;
		advanceBuffer(buffer, sizeof(T));
		return *(const T*)value;						//warning: no implicit endianes conversions done for this (unknown) types
	}
	
	const uint16_t readFromBuffer(MessageBuffer& buffer, identity<uint16_t> dummy) {
		const char* value=buffer.first;
		advanceBuffer(buffer, sizeof(uint16_t));
		return ntohs(*(const uint16_t*)value);			//uint16_t network byte order (bigEndian) to host byte order conversion
	}
	
	const uint32_t readFromBuffer(MessageBuffer& buffer, identity<uint32_t> dummy) {
		const char* value=buffer.first;
		advanceBuffer(buffer, sizeof(uint32_t));
		return ntohl(*(const uint32_t*)value);			//uint32_t network byte order (bigEndian) to host byte order conversion
	}
	
	const std::string readFromBuffer(MessageBuffer& buffer, size_t length) {
		const char* value=buffer.first;
		advanceBuffer(buffer, length);
		return std::string(value, length);
	}
	
	std::string& lengthEncoding(std::string& buffer, std::string value) {
		uint32_t length=htonl(value.length());
		buffer+=std::string((char*)&length, sizeof(uint32_t));
		buffer+=value;
		return buffer;
	}
	
	bool isSane(const std::string& key, bool throwException=true) {
		if(key.length()==0) {
			if(throwException)
				throw std::runtime_error("Cannot use empty key in BridgeMessage!");
			else
				return false;
		}
		return true;
	}

public:
	class proxy {
	private:
		BridgeMessage* _msg;
		std::string _key;
	public:
		proxy(BridgeMessage* msg, std::string& key) : _msg(msg), _key(key) { }
		operator const uint16_t () const {				//for use on RHS of assignment
			return _msg->get<uint16_t>(_key);
		}
		operator const uint32_t () const {				//for use on RHS of assignment
			return _msg->get<uint32_t>(_key);
		}
		operator const int () const {					//for use on RHS of assignment
			return _msg->get<int>(_key);
		}
		operator const bool () const {					//for use on RHS of assignment
			return _msg->get<bool>(_key);
		}
		///NOTE: add more conversions for RHS assignments here
		operator const char* () const {					//for use on RHS of assignment
			return _msg->get(_key).c_str();
		}
		operator const std::string () const {			//for use on RHS of assignment
			return _msg->get(_key);
		}
		/*template <typename T>
		operator const T& () const {					//for general use on RHS of assignment (creates ambiguities)
			return _msg->get<T>(_key);
		}*/
		template <typename T>
		proxy& operator=(const T& rhs) {				//for use on LHS of assignment
			_msg->set<T>(_key, rhs);
			return *this;
		}
		template <typename T>
		bool operator==(const T data) {
			return _msg->get<T>(_key) == data;
		}
		//mimic some std::string methods and behaviours used in our code
		const char* c_str() const {
			return _msg->get(_key).c_str();
		}
		size_t length() const {
			return _msg->get(_key).length();
		}
		size_t size() const {
			return _msg->get(_key).size();
		}
		bool operator==(const char* str) {
			return _msg->get(_key) == str;
		}
	};
	
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
	
	//STL containers will copy themselves
	BridgeMessage(const BridgeMessage& other) : _protocolVersion(other._protocolVersion), _data(other._data) { }
	
	//serialize message
	std::string toString() {
		uint16_t version = htons(_protocolVersion);
		std::string output = std::string((char*)&version, sizeof(version));
		//iterate over our key-value-pairs
		for(std::map<std::string, std::string>::iterator it=_data.begin(); it!=_data.end(); ++it) {
			lengthEncoding(output, it->first);
			lengthEncoding(output, it->second);
		}
		//mark message end (zero length key)
		lengthEncoding(output, "");
		return output;
	}
	
	operator std::string() {
		return toString();
	}
	
	template<typename T>
	void set(const std::string& key, const T& value) {
		isSane(key);		//throws exception on empty key
		_data[key]=toStr(value);
	}
	
	const std::map<std::string, std::string>& get() {
		return _data;
	}
	
	const std::string get(const std::string& key) {
		isSane(key);		//throws exception on empty key
		if(isset(key))
			return _data[key];
		return "";
	}
	
	template<typename T>
	const T get(const std::string& key) {
		return strTo<T>(get(key));
	}
	
	bool isset(const std::string& key) {
		isSane(key);		//throws exception on empty key
		return _data.find(key) != _data.end();
	}
	
	const proxy operator[](std::string& key) const {
		return proxy(const_cast<BridgeMessage*>(this), key);
	}

	proxy operator[](std::string& key) {
		return proxy(this, key);
	}
	
	const proxy operator[](std::string key) const {
		return proxy(const_cast<BridgeMessage*>(this), key);
	}

	proxy operator[](std::string key) {
		return proxy(this, key);
	}
	
	const proxy operator[](const char* key) const {
		std::string stringKey = std::string(key);
		return proxy(const_cast<BridgeMessage*>(this), stringKey);
	}

	proxy operator[](const char* key) {
		std::string stringKey = std::string(key);
		return proxy(this, stringKey);
	}
};
//stream output of BridgeMessage entries
std::ostream& operator<<(std::ostream& os, const BridgeMessage::proxy& msg) {
	os << msg.c_str();
	return os;
}

class MessageQueue {
private:
	std::queue<boost::shared_ptr<BridgeMessage> > _queue;
	RMutex _mutex;
	Monitor _cond;
	
	MessageQueue() { }
	
	MessageQueue(MessageQueue& other) : _queue(other._queue), _mutex(other._mutex), _cond(other._cond) { }
	
	void write(boost::shared_ptr<BridgeMessage> msg) {
		RScopeLock lock(_mutex);
		_queue.push(msg);
		_cond.broadcast();
	}
	
	void write(BridgeMessage& msg) {
		boost::shared_ptr<BridgeMessage> newMsg=boost::shared_ptr<BridgeMessage>(new BridgeMessage(msg));
		write(newMsg);
	}
	
	boost::shared_ptr<BridgeMessage> read() {
		return read(0);
	}
	
	boost::shared_ptr<BridgeMessage> read(uint32_t timeout_ms) {
		RScopeLock lock(_mutex);
		if(_queue.empty())
			_cond.wait(_mutex, timeout_ms);
		if(_queue.empty())		//timeout occured --> return NULL pointer
			return boost::shared_ptr<BridgeMessage>();
		boost::shared_ptr<BridgeMessage> msg=_queue.front();
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
	bool _terminated;
	
	TCPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue), _terminated(false) {
		start();
	}
	
	~TCPListener() {
		terminate();
	}
	
	void terminate() {
		_terminated=true;
		join();
	}
	
	void run() {
		try {
			char* buffer;
			uint32_t packetSize;
			while(!_terminated) {
				//use select() to timeout every second without data (and then check if _terminated == true)
				struct timeval time;
				#ifdef WIN32
					FD_SET receive;
				#else
					fd_set receive;
				#endif
				FD_ZERO(&receive);
				FD_SET(_socket, &receive);
				time.tv_sec=1;
				time.tv_usec=0;
				int retval=select(_socket+1, &receive, NULL, NULL, &time);
				if(retval==EBADF || retval==EINTR || retval==EINVAL || retval==ENOMEM)
					throw SocketException("Could not use select() on sockets (err="+toStr(retval)+")");
				if(FD_ISSET(_socket, &receive)) {
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
						boost::shared_ptr<BridgeMessage> msg;
						try {
							msg=boost::shared_ptr<BridgeMessage>(new BridgeMessage(buffer, packetSize));
							msg->set("_source", "TCPListener");
						} catch(std::exception& e) {
							free(buffer);
							throw e;
						}
						free(buffer);
						_queue.write(msg);
					}
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
		BridgeMessage terminationMessage;
		terminationMessage["type"]="internal";
		terminationMessage["cause"]="termination";
		terminationMessage["_source"]="TCPListener";
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
	bool _terminated;
	
	UDPListener(SOCKET socket, MessageQueue& queue) : _socket(socket), _queue(queue), _terminated(false) {
		start();
	}
	
	~UDPListener() {
		terminate();
	}
	
	void terminate() {
		_terminated=true;
		join();
	}
	
	void run() {
		char buffer[65536]="";
		try {
			while(!_terminated) {
				//use select() to timeout every second without data (and then check if _terminated == true)
				struct timeval time;
				#ifdef WIN32
					FD_SET receive;
				#else
					fd_set receive;
				#endif
				FD_ZERO(&receive);
				FD_SET(_socket, &receive);
				time.tv_sec=1;
				time.tv_usec=0;
				int retval=select(_socket+1, &receive, NULL, NULL, &time);
				if(retval==EBADF || retval==EINTR || retval==EINVAL || retval==ENOMEM)
					throw SocketException("Could not use select() on sockets (err="+toStr(retval)+")");
				if(FD_ISSET(_socket, &receive)) {
					size_t count=recv(_socket, buffer, sizeof(buffer), 0);
					if(count==-1)
						throw SocketException("recv() on udp socket failed");
					else {
						boost::shared_ptr<BridgeMessage> msg=boost::shared_ptr<BridgeMessage>(new BridgeMessage(buffer, count));
						msg->set("_source", "UDPListener");
						_queue.write(msg);
					}
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
		BridgeMessage terminationMessage;
		terminationMessage["type"]="internal";
		terminationMessage["cause"]="termination";
		terminationMessage["_source"]="UDPListener";
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
	RMutex _socketMutex;
	//our own publishers reproducing remote publishers
	std::map<bool, std::map<std::string, std::pair<Publisher*, std::map<std::string, Publisher*> > > > _knownPubs;
	RMutex _knownPubsMutex;
	//our own subscribers reproducing remote subscribers
	std::map<bool, std::map<std::string, std::pair<Subscriber*, std::map<std::string, Subscriber*> > > > _knownSubs;
	RMutex _knownSubsMutex;
	//count other local publishers sorted by channelName
	std::map<bool, std::map<std::string, uint32_t> > _seenPubs;
	RMutex _seenPubsMutex;
	bool _mainloopStarted;
	RMutex _mainloopStartedMutex;
	boost::shared_ptr<Node> _innerNode;
	enum dummy { UDP, TCP };

public:
	ProtocolHandler(SOCKET tcpSocket, SOCKET udpSocket, bool handleRTP) : _tcpSocket(tcpSocket), _udpSocket(udpSocket), _handleRTP(handleRTP), _shutdownDone(false), _mainloopStarted(false) {
		_tcpListener=new TCPListener(_tcpSocket, _queue);
		if(_handleRTP)
			_udpListener=new UDPListener(_udpSocket, _queue);
	}
	
	~ProtocolHandler() {
		shutdown();
		if(verbose)
			std::cout << "ProtocolHandler successfully destructed..." << std::endl;
	}
	
	void terminate() {
		if(verbose)
			std::cout << "ProtocolHandler.terminate() called..." << std::endl;
		shutdown();
	}
	
	void setNode(boost::shared_ptr<Node> innerNode) {
		if(_innerNode)
			throw std::runtime_error("You are not allowed to call setNode() twice!");
		_innerNode = innerNode;
	}
	
	// *** sender methods ***
	void send_pubAdded(std::string channelName, bool isRTP) {
		if(!_innerNode)		//only allow when we know our umundo node
			return;
		if(isRTP && !_handleRTP)		//ignore rtp messages when no udp transport is available
			return;
		BridgeMessage msg;
		msg["type"]="pubAdded";
		msg["channelName"]=channelName;
		msg["isRTP"]=isRTP;
		sendMessage(msg, TCP);
		{
			RScopeLock lock(_seenPubsMutex);
			_seenPubs[isRTP][channelName]++;
		}
	}
	
	void send_pubRemoved(std::string channelName, bool isRTP) {
		if(!_innerNode)					//only allow when we know our umundo node
			return;
		if(isRTP && !_handleRTP)		//ignore rtp messages when no udp transport is available
			return;
		BridgeMessage msg;
		msg["type"]="pubRemoved";
		msg["channelName"]=channelName;
		msg["isRTP"]=isRTP;
		sendMessage(msg, TCP);
		{
			RScopeLock lock(_seenPubsMutex);
			_seenPubs[isRTP][channelName]--;
			if(_seenPubs[isRTP][channelName] == 0) {		//last publisher seen --> unsubscribe all bridged subscribers for this channel
				RScopeLock lock(_knownSubsMutex);
				if(_knownSubs[msg["isRTP"]][msg["channelName"]].first!=NULL)
					for(uint32_t c=0; c<_knownSubs[msg["isRTP"]][msg["channelName"]].second.size()+1; c++)
						unsubscribe(msg["channelName"], msg["isRTP"]);
			}
		}
	}
	
	void send_subAdded(std::string channelName, bool isRTP) {
		if(!_innerNode)		//only allow when we know our umundo node
			return;
		if(isRTP && !_handleRTP)		//ignore rtp messages when no udp transport is available
			return;
		BridgeMessage msg;
		msg["type"]="subAdded";
		msg["channelName"]=channelName;
		msg["isRTP"]=isRTP;
		sendMessage(msg, TCP);
	}
	
	void send_subRemoved(std::string channelName, bool isRTP) {
		if(!_innerNode)		//only allow when we know our umundo node
			return;
		if(isRTP && !_handleRTP)		//ignore rtp messages when no udp transport is available
			return;
		BridgeMessage msg;
		msg["type"]="subRemoved";
		msg["channelName"]=channelName;
		msg["isRTP"]=isRTP;
		sendMessage(msg, TCP);
	}
	
	void send_data(std::string channelName, bool isRTP, Message* umundoMessage) {
		if(!_innerNode)		//only allow when we know our umundo node
			return;
		if(isRTP && !_handleRTP)		//ignore rtp messages when no udp transport is available
			return;
		{
			RScopeLock lock(_knownSubsMutex);
			//fetch data from all "silent" subscribers and discard it
			std::map<std::string, Subscriber*>::iterator subsIter=_knownSubs[isRTP][channelName].second.begin();
			while(subsIter!=_knownSubs[isRTP][channelName].second.end()) {
				while(subsIter->second->hasNextMsg())
					subsIter->second->getNextMsg();
				subsIter++;
			}
		}
		//"serialize" and send umundoMessage
		BridgeMessage msg;
		msg["type"]="data";
		msg["channelName"]=channelName;
		msg["isRTP"]=isRTP;
		msg["data"]=std::string(umundoMessage->data(), umundoMessage->size());
		uint32_t metadataCount=0;
		std::map<std::string, std::string>::const_iterator metaIter=umundoMessage->getMeta().begin();
		while(metaIter!=umundoMessage->getMeta().end()) {
			std::cout << "Setting metadata key '" << metaIter->first << "' to value '" << metaIter->second << std::endl;
			msg["metadataKey."+toStr(metadataCount)]=metaIter->first;
			msg["metadataValue."+toStr(metadataCount)]=metaIter->second;
			metadataCount++;
			metaIter++;
		}
		std::cout << "SENDING METADATA COUNT: " << metadataCount << std::endl;
		msg.set("metadataCount", metadataCount);
		sendMessage(msg, isRTP ? UDP : TCP);
	}
	
	// *** mainloop: read messages from the queue and process them (internal messages here, external messages in own private method) ***
	void mainloop() {
		if(!_innerNode)		//only allow when we know our umundo node
			throw std::runtime_error("Cannot start mainloop before setNode() was called!");
		if(_shutdownDone)
			throw std::runtime_error("You cannot call mainloop() after calling terminate()!");
		{
			RScopeLock lock(_mainloopStartedMutex);
			if(_mainloopStarted)
				throw std::runtime_error("You cannot call mainloop() twice!");
			_mainloopStarted = true;
		}
		time_t lastPing=time(NULL);
		while(!_shutdownDone) {
			//use read timeout to periodically send tcp and udp pings (to keep nat open etc.)
			boost::shared_ptr<BridgeMessage> msg=_queue.read(PING_INTERVAL*1000);		//wakeup every PING_INTERVAL seconds when idle
			if(_shutdownDone)
				break;
			if(difftime(time(NULL), lastPing)>=PING_INTERVAL) {		//difftime >= PING_INTERVAL --> ping remote host
				lastPing=time(NULL);
				BridgeMessage msg;
				msg["type"]="internal";
				msg["cause"]="ping";
				if(_handleRTP) {
					if(verbose)
						std::cout << "INFO: sending internal ping message on UDP channel..." << std::endl;
					sendMessage(msg, UDP);
				}
				if(verbose)
					std::cout << "INFO: sending internal ping message on TCP channel..." << std::endl;
				sendMessage(msg, TCP);
			}
			if(!msg)
				continue;
			
			//process internal messages
			if(msg->get("type")=="internal") {
				if(msg->get("cause")=="termination")
					break;
				else if(msg->get("cause")=="ping") {
					if(verbose)
						std::cout << "INFO: received internal ping message using " << msg->get("_source") << "..." << std::endl;
					continue;
				}
				else
					std::cout << "WARNING: got internal message with unknown cause '" << msg->get("cause") << "' and source '" << msg->get("_source") << "', ignoring it..." << std::endl;
			}
			
			std::cout << "D E B U G" << std::endl;
			std::map<std::string, std::string> kvp=msg->get();
			std::map<std::string, std::string>::iterator kvpIter = kvp.begin();
			while(kvpIter != kvp.end()) {
				std::cout << "KVP: " << kvpIter->first << " => " << kvpIter->second << std::endl;
				kvpIter++;
			}
			
			//process external messages
			if(processMessage(*msg)) {		//array notation only works with plain obkjects (not with shared_ptr containers)
				std::cout << "WARNING: unknown message type '" << msg->get("type") << "', protocol mismatch? --> closing connection..." << std::endl;
				break;
			}
		}
		if(verbose)
			std::cout << "ProtocolHandler lost connection..."<< std::endl;
		shutdown();
	}

private:
	/// *** process incoming messages ***
	bool processMessage(BridgeMessage& msg) {
		RScopeLock lock(_shutdownMutex);		//process only when no shutdown in progress
		if(_shutdownDone)
			return false;
		if(msg["type"]=="pubAdded") {
			RScopeLock lock(_knownPubsMutex);
			Publisher* ownPub;
			if(_knownPubs[msg["isRTP"]][msg["channelName"]].first==NULL) {		//first publisher --> register actual greeter to track subscriptions
				if(msg["isRTP"]) {
					RTPPublisherConfig pubConfig(1, 96);		//dummy values (timestamp increment 1, payload type dynamic [RFC3551])
					ownPub = new Publisher(Publisher::RTP, msg["channelName"], &pubConfig);
				} else {
					ownPub = new Publisher(Publisher::ZEROMQ, msg["channelName"]);
				}
				GlobalGreeter* ownGreeter = new GlobalGreeter(ownPub, shared_from_this());
				ownPub->setGreeter(ownGreeter);
				_knownPubs[msg["isRTP"]][msg["channelName"]].first = ownPub;
				if(verbose)
					std::cout << "Created new 'normal' LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " publisher " << ownPub->getUUID() << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
			} else {
				if(msg["isRTP"]) {
					RTPPublisherConfig pubConfig(1, 96);		//dummy values (timestamp increment 1, payload type dynamic [RFC3551])
					ownPub = new Publisher(Publisher::RTP, msg["channelName"], &pubConfig);
				} else {
					ownPub = new Publisher(Publisher::ZEROMQ, msg["channelName"]);
				}
				_knownPubs[msg["isRTP"]][msg["channelName"]].second[ownPub->getUUID()] = ownPub;
				if(verbose)
				std::cout << "Created new 'silent' LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " publisher " << ownPub->getUUID() << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
			}
			_innerNode->addPublisher(*ownPub);
		
		} else if(msg["type"]=="pubRemoved") {
			RScopeLock lock(_knownPubsMutex);
			if(_knownPubs[msg["isRTP"]][msg["channelName"]].second.size()) {
				Publisher* pub = _knownPubs[msg["isRTP"]][msg["channelName"]].second.begin()->second;
				std::string localUUID = pub->getUUID();
				_knownPubs[msg["isRTP"]][msg["channelName"]].second.erase(localUUID);
				delete pub;
				if(verbose)
					std::cout << "Removed 'silent' LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " publisher " << localUUID << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
			} else {
				if(_knownPubs[msg["isRTP"]].find(msg["channelName"])==_knownPubs[msg["isRTP"]].end() || _knownPubs[msg["isRTP"]][msg["channelName"]].first==NULL) {
					if(verbose)
						std::cout << "LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " publisher for channel '" << msg["channelName"] << "' unknown, ignoring removal!" << std::endl;
				} else {
					Publisher* pub = _knownPubs[msg["isRTP"]][msg["channelName"]].first;
					std::string localUUID = pub->getUUID();
					GlobalGreeter* greeter = dynamic_cast<GlobalGreeter*>(pub->getGreeter());
					pub->setGreeter(NULL);
					_innerNode->removePublisher(*pub);
					_knownPubs[msg["isRTP"]][msg["channelName"]].first = NULL;
					_knownPubs[msg["isRTP"]].erase(msg["channelName"]);
					delete pub;
					delete greeter;
					if(verbose)
						std::cout << "Removed 'normal' LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " publisher " << localUUID << " for REMOTE publisher on channel '" << msg["channelName"] << "'" << std::endl;
				}
			}
		
		} else if(msg["type"]=="subAdded") {
			Subscriber* sub;
			{
				RScopeLock lock(_knownSubsMutex);
				if(_knownSubs[msg["isRTP"]][msg["channelName"]].first==NULL) {		//first subscriber --> register actual receiver to receive the data
					GlobalReceiver* receiver = new GlobalReceiver(msg["channelName"], msg["isRTP"], shared_from_this());
					sub = new Subscriber(msg["isRTP"] ? Subscriber::RTP : Subscriber::ZEROMQ, msg["channelName"], receiver);
					_knownSubs[msg["isRTP"]][msg["channelName"]].first = sub;
				} else {
					sub = new Subscriber(msg["isRTP"] ? Subscriber::RTP : Subscriber::ZEROMQ, msg["channelName"]);
					_knownSubs[msg["isRTP"]][msg["channelName"]].second[sub->getUUID()] = sub;
				}
			}
			_innerNode->addSubscriber(*sub);
			if(verbose)
				std::cout << "Created new LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " subscriber " << sub->getUUID() << " for REMOTE subscriber on channel '" << msg["channelName"] << "'" << std::endl;
		
		} else if(msg["type"]=="subRemoved") {
			std::string localUUID;
			unsubscribe(msg["channelName"], msg["isRTP"]);
			if(verbose && localUUID.length())
				std::cout << "Removed LOCAL " << (msg["isRTP"] ? "RTP" : "ZMQ") << " subscriber " << localUUID << " for REMOTE subscriber on channel '" << msg["channelName"] << "'" << std::endl;
		
		} else if(msg["type"]=="data") {
			Message* umundoMessage = new Message(msg["data"].c_str(), msg["data"].length());
			uint32_t metadataCount = msg.get<uint32_t>("metadataCount");
			for(uint32_t c=0; c<metadataCount; c++)
			{
				std::cout << "Setting metadata key '" << msg["metadataKey."+toStr(c)] << "' to value '" << msg["metadataValue."+toStr(c)] << "'" << std::endl;
				umundoMessage->putMeta(msg["metadataKey."+toStr(c)], msg["metadataValue."+toStr(c)]);
			}
			std::cout << "RECEIVING METADATA COUNT: " << metadataCount << std::endl;
			{
				RScopeLock lock(_knownPubsMutex);
				if(_knownPubs[msg["isRTP"]].find(msg["channelName"])!=_knownPubs[msg["isRTP"]].end())
					_knownPubs[msg["isRTP"]][msg["channelName"]].first->send(umundoMessage);
			}
			delete(umundoMessage);
		
		}
		else
			return true;
		return false;
	}
	
	std::string unsubscribe(std::string channelName, bool isRTP) {
		RScopeLock lock(_knownSubsMutex);
		std::string localUUID;
		if(_knownSubs[isRTP][channelName].second.empty()) {		//last subscriber removed --> remove our master subscriber
			if(_knownSubs[isRTP][channelName].first==NULL) {
				if(verbose)
					std::cout << "REMOTE " << (isRTP ? "RTP" : "ZMQ") << " subscriber for channel '" << channelName << "' unknown, ignoring removal!" << std::endl;
			} else {
				Subscriber* sub=_knownSubs[isRTP][channelName].first;
				localUUID=sub->getUUID();
				Receiver* receiver = sub->getReceiver();
				sub->setReceiver(NULL);
				_innerNode->removeSubscriber(*sub);
				_knownSubs[isRTP][channelName].first=NULL;
				delete sub;
				delete receiver;
			}
		} else {			//this was not our last subscriber --> remove only "silent" subscriber
			Subscriber* sub=_knownSubs[isRTP][channelName].second.begin()->second;
			localUUID=sub->getUUID();
			_innerNode->removeSubscriber(*sub);
			_knownSubs[isRTP][channelName].second.erase(sub->getUUID());
			delete sub;
		}
		return localUUID;
	}
	
	void shutdown() {
		RScopeLock lock(_shutdownMutex);
		if(_shutdownDone)
			return;
		_shutdownDone=true;
		
		if(verbose)
			std::cout << "Shutting down ProtocolHandler..." << std::endl;
		
		//wakeup mainloop (it will detect _shutdownDone == true and terminate)
		BridgeMessage msg;
		msg["type"]="internal";
		msg["_source"]="ProtocolHandler";
		msg["cause"]="termination";
		_queue.write(msg);
		
		if(_innerNode) {
			//remove all publishers
			{
				RScopeLock lock(_knownPubsMutex);
				bool isRTP=false;
				do {
					std::map<std::string, std::pair<Publisher*, std::map<std::string, Publisher*> > >::iterator pubIter = _knownPubs[isRTP].begin();
					while(pubIter != _knownPubs[isRTP].end()) {		//loop over all channels
						if(pubIter->second.second.size()) {
							std::map<std::string, Publisher*>::iterator innerPubIter = pubIter->second.second.begin();
							while(innerPubIter != pubIter->second.second.end()) {		//loop over all "silent" publishers
								delete innerPubIter->second;
								innerPubIter++;
							}
							pubIter->second.second.clear();
						}
						if(pubIter->second.first!=NULL) {		//remove "normal" publisher
							Publisher* pub = pubIter->second.first;
							GlobalGreeter* greeter = dynamic_cast<GlobalGreeter*>(pub->getGreeter());
							pub->setGreeter(NULL);
							_innerNode->removePublisher(*pub);
							_knownPubs[isRTP][pubIter->first].first = NULL;
							delete pub;
							delete greeter;
						}
						pubIter++;
					}
				isRTP=!isRTP;
				} while(!isRTP);
				_knownPubs.clear();
			}
			
			//remove all subscribers
			{
				RScopeLock lock(_knownSubsMutex);
				bool isRTP=false;
				do {
					std::map<std::string, std::pair<Subscriber*, std::map<std::string, Subscriber*> > >::iterator subIter = _knownSubs[isRTP].begin();
					while(subIter != _knownSubs[isRTP].end()) {
						for(uint32_t c=0; c<subIter->second.second.size(); c++)		//remove all "silent" subscribers
							unsubscribe(subIter->first, isRTP);
						if(subIter->second.first!=NULL)									//remove master subscriber
							unsubscribe(subIter->first, isRTP);
						subIter++;
					}
					isRTP=!isRTP;
				} while(!isRTP);
			}
		}
		
		//lock "sockets" to prevent sendMessage() calls while closing the sockets 
		{
			RScopeLock lock(_socketMutex);
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
		_innerNode.reset();
	}
	
	void sendMessage(BridgeMessage& msg, dummy channel) {
		RScopeLock lock(_socketMutex);
		if(_shutdownDone)		//only send messages when active
			return;
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

GlobalReceiver::GlobalReceiver(std::string channelName, bool isRTP, boost::shared_ptr<ProtocolHandler> handler) : _channelName(channelName), _isRTP(isRTP), _handler(handler) { }

void GlobalReceiver::receive(Message* msg) {
	_handler->send_data(_channelName, _isRTP, msg);
}

GlobalGreeter::GlobalGreeter(Publisher* pub, boost::shared_ptr<ProtocolHandler> handler) : _pub(pub), _handler(handler) { }

void GlobalGreeter::welcome(Publisher& pub, const SubscriberStub& subStub) {
	if(verbose)
		std::cout << "Got new LOCAL " << (subStub.isRTP() ? "RTP" : "ZMQ") << " subscriber: " << subStub << " to publisher " << *_pub << std::endl;
	if(verbose)
		std::cout << "--> subscribing on REMOTE side..." << std::endl;
	_handler->send_subAdded(_pub->getChannelName(), subStub.isRTP());
}

void GlobalGreeter::farewell(Publisher& pub, const SubscriberStub& subStub) {
	if(verbose)
		std::cout << "Removed LOCAL " << (subStub.isRTP() ? "RTP" : "ZMQ") << " subscriber: " << subStub << " to publisher " << *_pub << std::endl;
	if(verbose)
		std::cout << "--> unsubscribing on REMOTE side..." << std::endl;
	_handler->send_subRemoved(pub.getChannelName(), subStub.isRTP());
}

//monitor addition and removal of new publishers and publish the same channel on the other side
class PubMonitor : public ResultSet<PublisherStub> {
private:
	boost::shared_ptr<ProtocolHandler> _handler;

public:
	PubMonitor(boost::shared_ptr<ProtocolHandler> handler) : _handler(handler) { }
	
	void added(PublisherStub pubStub) {
		if(verbose)
			std::cout << "Got new LOCAL " << (pubStub.isRTP() ? "RTP" : "ZMQ") << " publisher: " << pubStub << std::endl;
		if(verbose)
			std::cout << "--> sending PUB_ADDED for channel '" << pubStub.getChannelName() << "' through bridge..." << std::endl;
		_handler->send_pubAdded(pubStub.getChannelName(), pubStub.isRTP());
	}
	
	void removed(PublisherStub pubStub) {
		if(verbose)
			std::cout << "Got removal of LOCAL " << (pubStub.isRTP() ? "RTP" : "ZMQ") << " publisher: " << pubStub << std::endl;
		if(verbose)
			std::cout << "--> sending PUB_REMOVED for channel '" << pubStub.getChannelName() << "' through bridge..." << std::endl;
		_handler->send_pubRemoved(pubStub.getChannelName(), pubStub.isRTP());
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
		if(setsockopt(_tcpSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(reuseaddr))==-1)
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
					std::cout << "WARNING: could not send data on udp socket" << std::endl;
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
				} catch(...) {
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

//test message serialisation/deserialisation (class BridgeMessage)
void test_BridgeMessage() {
	BridgeMessage msg;
	uint32_t abcd=8;
	msg["abcd"]=abcd;									//this is automatically saved as string (via toStr(8))
	msg["zweiundvierzig"]=42;							//this is converted to string, too
	msg.set("setter", 42);								//this is also converted to string
	msg.set("number", "44");							//this is already a string
	msg["xyz"]="halloWelt";								//this is also already a string (but representing a number --> can be converted to a number)
	msg["negativ"]=-42;									//this is a negative number
	msg["float"]=-42.42;								//and this is a floating point number
	msg["true"]=true;									//this is a boolean (true)
	msg["false"]=false;									//this is a boolean (false)
	
	std::string serialized=msg.toString();								//serialize message
	BridgeMessage newMsg(serialized.c_str(), serialized.length());		//now deserialize our message again
	
	assert(newMsg.get("xyz") == "halloWelt");			//getter access
	assert(newMsg["xyz"] == "halloWelt");				//array like access
	assert(newMsg.get<uint32_t>("abcd") == 8);			//getter access via template (explicit strTo<uint32_t> conversion)
	uint32_t tmp=newMsg["abcd"];						//array like access via automatic conversion to uint32_t
	assert(tmp == 8);
	assert(newMsg["zweiundvierzig"] == 42);				//array like access via automatic conversion
	assert(newMsg["setter"] == 42);						//array like access via automatic conversion
	assert(newMsg["number"] == "44");					//array like access without conversion
	assert(newMsg["number"] == 44);						//array like access via automatic conversion
	assert(newMsg["negativ"] == -42);					//array like access via automatic conversion
	uint32_t tmp2 = newMsg.get<uint32_t>("negativ");	//getter access via template (explicit strTo<uint32_t> conversion)
	assert(tmp2 == (uint32_t)-42);
	assert(newMsg["float"] == -42.42);					//array like access via automatic conversion
	assert(newMsg["true"] == true);						//array like access via automatic conversion
	assert(newMsg["false"] == false);					//array like access via automatic conversion
}

//test internal classes
void test_All() {
	std::cout << "Testing BridgeMessage (de-)serialisation..." << std::endl;
	test_BridgeMessage();
	std::cout << "All tests passed successfully..." << std::endl;
}

int main(int argc, char** argv) {
	int option;
	int listen = 0;
	std::string remoteIP = "";
	uint16_t remotePort = 0;
	Connector* connector;
	boost::shared_ptr<ProtocolHandler> handler;
	
	while((option = getopt(argc, argv, "tvd:l:c:")) != -1) {
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
			case 't':
				test_All();
				return 0;
			default:
				printUsageAndExit();
				break;
		}
	}
	if(optind < argc || (listen == 0 && remotePort == 0))
		printUsageAndExit();
	
	//construct discovery (must be done here, because destruction of the discovery object provokes a bug which prevents subsequent constructions of new discovery objects)
	MDNSDiscoveryOptions mdnsOpts;
	if(domain)
		mdnsOpts.setDomain(domain);
	Discovery disc(Discovery::MDNS, &mdnsOpts);
	
	while(true)
	{
		try {
			if(listen) {
				if(verbose)
					std::cout << "Listening at tcp and udp port " << listen << std::endl;
				connector = new Connector(listen);
			} else {
				if(verbose)
					std::cout << "Connecting to remote bridge instance at " << remoteIP << ":" << remotePort << std::endl;
				connector = new Connector(remoteIP, remotePort);
			}
			handler=connector->waitForConnection();
			delete connector;
			
			//construct inner node
			boost::shared_ptr<Node> innerNode = boost::shared_ptr<Node>(new Node());
			
			//construct monitor and associate it to our node
			PubMonitor* innerMonitor = new PubMonitor(handler);
			innerNode->addPublisherMonitor(innerMonitor);
			
			//add inner node to protocol handler
			handler->setNode(innerNode);
			
			//add inner node to discovery
			disc.add(*innerNode);
			
			//start bridge mainloop
			handler->mainloop();
			
			//remove PubMonitor before we leave this scope and destruct our Node and handler
			innerNode->clearPublisherMonitors();
			delete innerMonitor;
		} catch(std::runtime_error& e) {
			std::cout << "Got runtime_error with message '" << e.what() << "'" << std::endl;
		}
		if(listen)
			std::cout << "Listening for new connections in 5 seconds..." << std::endl;
		else
			std::cout << "Reconnecting in 5 seconds..." << std::endl;
		Thread::sleepMs(5000);
	}
}