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

#ifndef PUBLISHER_H_F3M1RWLN
#define PUBLISHER_H_F3M1RWLN

#include "umundo/common/Common.h"
#include "umundo/common/EndPoint.h"
#include "umundo/common/Implementation.h"

namespace umundo {

class NodeStub;
class Message;
class Publisher;

/**
 * Wait for new subscribers and welcome them.
 *
 * A greeter can be registered at a publisher to send a message, whenever a new subscriber
 * is added to the publisher.
 */
class DLLEXPORT Greeter {
public:
	virtual ~Greeter() {};
	virtual void welcome(Publisher*, const string nodeId, const string subId) {};
	virtual void farewell(Publisher*, const string nodeId, const string subId) {};
};

class DLLEXPORT PublisherConfig : public Configuration {
public:
	shared_ptr<Configuration> create();
	virtual ~PublisherConfig() {}

	string channelName;
	string transport;
	uint16_t port;
};

/**
 * Representation of a remote Publisher.
 */
class DLLEXPORT PublisherStub : public EndPoint {
public:
	PublisherStub() {}
	virtual ~PublisherStub() {}

	/** @name Functionality of local *and* remote Publishers */
	//@{
	virtual const string& getChannelName() const      {
		return _channelName;
	}
	virtual void setChannelName(string channelName)   {
		_channelName = channelName;
	}
	virtual const string& getHost() const             {
		return _host;
	}
	virtual void setHost(string host)                 {
		_host = host;
	}
	virtual const string& getDomain() const           {
		return _domain;
	}
	virtual void setDomain(string domain)             {
		_domain = domain;
	}
	virtual const string& getUUID() const           {
		return _uuid;
	}
	virtual void setUUID(string uuid)             {
		_uuid = uuid;
	}
	//@}

protected:
	string _channelName;
	string _host;
	string _domain;
	string _uuid;
};

/**
 * Publisher implementor basis class (bridge pattern)
 */
class DLLEXPORT PublisherImpl : public Implementation, public PublisherStub {
public:
	PublisherImpl() : _greeter(NULL) {}
	virtual ~PublisherImpl();

	virtual void send(Message* msg) = 0;

	/** @name Optional subscriber awareness */
	//@{
	virtual int waitForSubscribers(int count, int timeoutMs)        {
		return -1;
	}
	virtual void setGreeter(Greeter* greeter)        {
		_greeter = greeter;
	}
	//@}

protected:
	/** @name Optional subscriber awareness */
	//@{
	virtual void addedSubscriber(const string, const string)   {
		/* Ignore or overwrite */
	}
	virtual void removedSubscriber(const string, const string) {
		/* Ignore or overwrite */
	}
	//@}

	Greeter* _greeter;
	friend class Publisher;
};

/**
 * Abstraction for publishing byte-arrays on channels (bridge pattern).
 *
 * We need to overwrite everything and use the concrete implementors fields.
 */
class DLLEXPORT Publisher : public PublisherStub {
public:
	Publisher(const string& channelName);
	virtual ~Publisher();

	/** @name Functionality of local Publishers */
	//@{
	void send(Message* msg)                        {
		_impl->send(msg);
	}
	void send(const char* data, size_t length);
	int waitForSubscribers(int count, int timeoutMs = 0)              {
		return _impl->waitForSubscribers(count, timeoutMs);
	}
	void setGreeter(Greeter* greeter)    {
		return _impl->setGreeter(greeter);
	}
	//@}

	/** @name Overwrite PublisherStub */
	//@{
	virtual const string& getChannelName() const      {
		return _impl->getChannelName();
	}
	virtual void setChannelName(string channelName)   {
		_impl->setChannelName(channelName);
	}
	//@}

	/** @name Overwrite EndPoint */
	virtual const string& getIP() const         {
		return _impl->getIP();
	}
	virtual void setIP(string ip)               {
		_impl->setIP(ip);
	}
	virtual const string& getTransport() const  {
		return _impl->getTransport();
	}
	virtual void setTransport(string transport) {
		_impl->setTransport(transport);
	}
	virtual uint16_t getPort() const            {
		return _impl->getPort();
	}
	virtual void setPort(uint16_t port)         {
		_impl->setPort(port);
	}
	virtual bool isRemote() const               {
		return _impl->isRemote();
	}
	virtual void setRemote(bool remote)         {
		_impl->setRemote(remote);
	}
	virtual const string& getHost() const       {
		return _impl->getHost();
	}
	virtual void setHost(string host)           {
		_impl->setHost(host);
	}
	virtual const string& getDomain() const     {
		return _impl->getDomain();
	}
	virtual void setDomain(string domain)       {
		_impl->setDomain(domain);
	}
	//@{

protected:
	void addedSubscriber(const string nodeId, const string subId)   {
		_impl->addedSubscriber(nodeId, subId);
	}
	void removedSubscriber(const string nodeId, const string subId) {
		_impl->removedSubscriber(nodeId, subId);
	}
	shared_ptr<PublisherImpl> _impl;
	shared_ptr<PublisherConfig> _config;
	friend class Node;
};

}


#endif /* end of include guard: PUBLISHER_H_F3M1RWLN */
