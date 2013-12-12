/**
 *  @file
 *  @brief      Classes and interfaces to receive data from channels.
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

#ifndef SUBSCRIBER_H_J64J09SP
#define SUBSCRIBER_H_J64J09SP

#include "umundo/common/Common.h"
#include "umundo/common/UUID.h"
#include "umundo/connection/Publisher.h"
#include "umundo/connection/SubscriberStub.h"
#include "umundo/common/EndPoint.h"
#include "umundo/common/Implementation.h"

#include <list>

namespace umundo {

class NodeStub;
class Message;
class Publisher;
class PublisherStub;

/**
 * Interface for client classes to get byte-arrays from subscribers.
 */
class DLLEXPORT Receiver {
public:
	virtual ~Receiver() {}
	virtual void receive(Message* msg) = 0;
	friend class Subscriber;
};

class DLLEXPORT SubscriberConfig : public Options {
public:
	std::string getType() { return "SubscriberConfig"; }
	std::string channelName;
	std::string uuid;
};

/**
 * Subscriber implementor basis class (bridge pattern).
 */
class DLLEXPORT SubscriberImpl : public SubscriberStubImpl, public Implementation {
public:
	SubscriberImpl();
	virtual ~SubscriberImpl();

	virtual Receiver* getReceiver() {
		return _receiver;
	}

	virtual void setReceiver(Receiver* receiver) = 0;

	std::map<std::string, PublisherStub> getPublishers() {
		return _pubs;
	}
	virtual void setChannelName(const std::string& channelName) {
		_channelName = channelName;
	}

	virtual void added(const PublisherStub& pub, const NodeStub& node) = 0;
	virtual void removed(const PublisherStub& pub, const NodeStub& node) = 0;

	virtual Message* getNextMsg() = 0;
	virtual bool hasNextMsg() = 0;

	virtual bool matches(const std::string& channelName) = 0;

	static int instances;

protected:
	Receiver* _receiver;
	std::map<std::string, PublisherStub> _pubs;
};


/**
 * Subscriber abstraction (bridge pattern).
 *
 * We need to overwrite everything to use the concrete implementors functions. The preferred
 * constructor is the Subscriber(string channelName, Receiver* receiver) one, the unqualified
 * constructor without a receiver and the setReceiver method are required for Java as we cannot
 * inherit publishers while being its receiver at the same time as is used for the TypedSubscriber.
 */
class DLLEXPORT Subscriber : public SubscriberStub {
public:
	Subscriber() : _impl() {}
	explicit Subscriber(const SubscriberStub& stub) : _impl(boost::static_pointer_cast<SubscriberImpl>(stub.getImpl())) {}
	Subscriber(const std::string& channelName);
	Subscriber(const std::string& channelName, Receiver* receiver);
	Subscriber(boost::shared_ptr<SubscriberImpl> const impl) : SubscriberStub(impl), _impl(impl) { }
	Subscriber(const Subscriber& other) : SubscriberStub(other._impl), _impl(other._impl) { }
	virtual ~Subscriber();

	operator bool() const {
		return _impl;
	}
	bool operator< (const Subscriber& other) const {
		return _impl < other._impl;
	}
	bool operator==(const Subscriber& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const Subscriber& other) const {
		return _impl != other._impl;
	}

	Subscriber& operator=(const Subscriber& other) {
		_impl = other._impl;
		SubscriberStub::_impl = _impl;
		return *this;
	} // operator=

	void setReceiver(Receiver* receiver) {
		_impl->setReceiver(receiver);
	}

	virtual void setChannelName(const std::string& channelName)  {
		_impl->setChannelName(channelName);
	}

	virtual Message* getNextMsg() {
		return _impl->getNextMsg();
	}

	virtual bool hasNextMsg() {
		return _impl->hasNextMsg();
	}

	virtual bool matches(const std::string& channelName) {
		return _impl->matches(channelName);
	}

	std::map<std::string, PublisherStub> getPublishers()             {
		return _impl->getPublishers();
	}
	bool isSubscribedTo(const std::string& uuid) {
		std::map<std::string, PublisherStub> pubs = _impl->getPublishers();
		return pubs.find(uuid) != pubs.end();
	}

	void added(const PublisherStub& pub, const NodeStub& node) {
		_impl->added(pub, node);
	}
	void removed(const PublisherStub& pub, const NodeStub& node) {
		_impl->removed(pub, node);
	}

	void suspend() {
		return _impl->suspend();
	}
	void resume() {
		return _impl->resume();
	}

	boost::shared_ptr<SubscriberImpl> getImpl() const {
		return _impl;
	}

protected:

	boost::shared_ptr<SubscriberImpl> _impl;
	boost::shared_ptr<SubscriberConfig> _config;
	friend class Node;
};

}


#endif /* end of include guard: SUBSCRIBER_H_J64J09SP */
