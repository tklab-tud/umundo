/**
 *  @file
 *  @brief      Classes and interfaces to send data to channels.
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
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
 *  @endcond
 */

#ifndef PUBLISHER_H_F3M1RWLN
#define PUBLISHER_H_F3M1RWLN

#include "umundo/common/Common.h"
#include "umundo/common/UUID.h"
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
	virtual void welcome(Publisher, const string& nodeId, const string& subId) = 0;
	virtual void farewell(Publisher, const string& nodeId, const string& subId) = 0;
};

class DLLEXPORT PublisherConfig : public Configuration {
public:
	shared_ptr<Configuration> create();
	virtual ~PublisherConfig() {}

	string channelName;
	string transport;
	uint16_t port;
};

class PublisherStubImpl : public EndPointImpl {
public:
	PublisherStubImpl() : _uuid(UUID::getUUID()) {}
	virtual std::string getChannelName() const            {
		return _channelName;
	}
	virtual void setChannelName(const std::string& channelName) {
		_channelName = channelName;
	}

	virtual std::string getUUID() const            {
		return _uuid;
	}
	virtual void setUUID(const std::string& uuid) {
		_uuid = uuid;
	}

protected:
	std::string _channelName;
	std::string _uuid;
};

/**
 * Publisher implementor basis class (bridge pattern)
 */
class DLLEXPORT PublisherImpl : public PublisherStubImpl, public Implementation {
public:
	PublisherImpl();
	virtual ~PublisherImpl();

	virtual void send(Message* msg) = 0;
	void putMeta(const string& key, const string& value) {
		_mandatoryMeta[key] = value;
	}

	/** @name Optional subscriber awareness */
	//@{
	virtual int waitForSubscribers(int count, int timeoutMs)        {
		return -1;
	}
	virtual void setGreeter(Greeter* greeter)        {
		_greeter = greeter;
	}
	std::set<string> getSubscriberUUIDs()             {
		return _subUUIDs;
	}

	//@}

	static int instances;

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

	std::map<string, string> _mandatoryMeta;
	std::set<string> _subUUIDs;
	Greeter* _greeter;
	friend class Publisher;

};

/**
 * Representation of a remote Publisher.
 */
class DLLEXPORT PublisherStub : public EndPoint {
public:
	PublisherStub() : _impl() { }
	PublisherStub(boost::shared_ptr<PublisherStubImpl> const impl) : EndPoint(impl), _impl(impl) { }
	PublisherStub(const PublisherStub& other) : EndPoint(other._impl), _impl(other._impl) { }

	operator bool() const {
		return _impl;
	}
	bool operator< (const PublisherStub& other) const {
		return _impl < other._impl;
	}
	bool operator==(const PublisherStub& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const PublisherStub& other) const {
		return _impl != other._impl;
	}

	PublisherStub& operator=(const PublisherStub& other) {
		_impl = other._impl;
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	boost::shared_ptr<PublisherStubImpl> getImpl() const {
		return _impl;
	}

	/** @name Functionality of local *and* remote Publishers */
	//@{
	virtual const std::string getChannelName() const      {
		return _impl->getChannelName();
	}
	virtual const std::string getUUID() const             {
		return _impl->getUUID();
	}
	//@}

protected:
	boost::shared_ptr<PublisherStubImpl> _impl;
};

#if 0
std::ostream & operator<<(std::ostream &os, const PublisherStub& pubStub) {
	os << "PublisherStub:" << std::endl;
	os << "\tchannelName: " << pubStub.getChannelName() << std::endl;
	os << "\tuuid: " << pubStub.getUUID() << std::endl;
	os << static_cast<EndPoint>(pubStub);
	return os;
}
#endif

/**
 * Abstraction for publishing byte-arrays on channels (bridge pattern).
 *
 * We need to overwrite everything and use the concrete implementors fields.
 */
class DLLEXPORT Publisher : public PublisherStub {
public:
	Publisher() : _impl() {}
	Publisher(const std::string& channelName);
	Publisher(const std::string& channelName, Greeter* greeter);
	Publisher(const boost::shared_ptr<PublisherImpl> impl) : PublisherStub(impl), _impl(impl) { }
	Publisher(const Publisher& other) : PublisherStub(other._impl), _impl(other._impl) { }
	virtual ~Publisher();

	operator bool() const {
		return _impl;
	}
	bool operator< (const Publisher& other) const {
		return _impl < other._impl;
	}
	bool operator==(const Publisher& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const Publisher& other) const {
		return _impl != other._impl;
	}

	Publisher& operator=(const Publisher& other) {
		_impl = other._impl;
		PublisherStub::_impl = _impl;
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	/** @name Functionality of local Publishers */
	//@{
	void send(Message* msg)                              {
		_impl->send(msg);
	}
	void send(const char* data, size_t length);
	int waitForSubscribers(int count, int timeoutMs = 0) {
		return _impl->waitForSubscribers(count, timeoutMs);
	}
	void setGreeter(Greeter* greeter)                    {
		return _impl->setGreeter(greeter);
	}
	void putMeta(const string& key, const string& value) {
		return _impl->putMeta(key, value);
	}
	std::set<string> getSubscriberUUIDs() const          {
		return _impl->getSubscriberUUIDs();
	}
	bool isPublishingTo(const string& uuid) {
		return _impl->getSubscriberUUIDs().find(uuid) != _impl->getSubscriberUUIDs().end();
	}
	//@}

	void suspend() {
		return _impl->suspend();
	}
	void resume() {
		return _impl->resume();
	}

	void addedSubscriber(const string nodeId, const string subId)   {
		_impl->addedSubscriber(nodeId, subId);
	}
	void removedSubscriber(const string nodeId, const string subId) {
		_impl->removedSubscriber(nodeId, subId);
	}

protected:
	shared_ptr<PublisherImpl> _impl;
	shared_ptr<PublisherConfig> _config;
	friend class Node;
};

#if 0
std::ostream & operator<<(std::ostream &os, const Publisher& pub) {
	os << "Publisher:" << std::endl;
	os << "\tpublishing to: ";
	std::set<string> subIds = pub.getSubscriberUUIDs();
	std::set<string>::iterator subIdIter = subIds.begin();
	while(subIdIter != subIds.end()) {
		os << *subIdIter << ", ";
		subIdIter++;
	}
	os << std::endl;
	os << static_cast<PublisherStub>(pub);
	return os;
}
#endif

}


#endif /* end of include guard: PUBLISHER_H_F3M1RWLN */
