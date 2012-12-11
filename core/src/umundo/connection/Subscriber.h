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
#include "umundo/common/ResultSet.h"
#include "umundo/common/Implementation.h"
#include "umundo/thread/Thread.h"

#include <list>

namespace umundo {

class Message;
class SubscriberImpl;

/**
 * Interface for client classes to get byte-arrays from subscribers.
 */
class DLLEXPORT Receiver {
public:
	virtual ~Receiver() {}
	virtual void receive(Message* msg) = 0;
	friend class Subscriber;
};

class DLLEXPORT SubscriberConfig : public Configuration {
public:
	virtual ~SubscriberConfig() {}
	shared_ptr<Configuration> create();
	string channelName;
	string uuid;
};

class DLLEXPORT SubscriberStubImpl : public EndPointImpl {
public:
	SubscriberStubImpl() : _uuid(UUID::getUUID()) {}
	virtual ~SubscriberStubImpl() {}
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
 * Subscriber implementor basis class (bridge pattern).
 */
class DLLEXPORT SubscriberImpl : public SubscriberStubImpl, public Implementation, public ResultSet<PublisherStub> {
public:
	SubscriberImpl();
	virtual ~SubscriberImpl();

	virtual Receiver* getReceiver() {
		return _receiver;
	}

	virtual void setReceiver(Receiver* receiver) = 0;

	std::set<string> getPublisherUUIDs() {
		return _pubUUIDs;
	}
	virtual void setChannelName(const std::string& channelName) {
		_channelName = channelName;
	}

	virtual Message* getNextMsg() = 0;
	virtual bool hasNextMsg() = 0;

	static int instances;

protected:
	Receiver* _receiver;
	std::set<string> _pubUUIDs;
};

/**
 * Subscriber implementor basis class (bridge pattern).
 */
class DLLEXPORT SubscriberStub {
public:
	virtual ~SubscriberStub() {}
	SubscriberStub() : _impl() { }
	SubscriberStub(boost::shared_ptr<SubscriberStubImpl> const impl) : _impl(impl) { }
	SubscriberStub(const SubscriberStub& other) : _impl(other._impl) { }

	operator bool() const {
		return _impl;
	}
	bool operator< (const SubscriberStub& other) const {
		return _impl < other._impl;
	}
	bool operator==(const SubscriberStub& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const SubscriberStub& other) const {
		return _impl != other._impl;
	}

	SubscriberStub& operator=(const SubscriberStub& other) {
		_impl = other._impl;
		return *this;
	} // operator=

	virtual const std::string getChannelName() const     {
		return _impl->getChannelName();
	}
	virtual const std::string getUUID() const            {
		return _impl->getUUID();
	}

	shared_ptr<SubscriberStubImpl> getImpl() const {
		return _impl;
	}

protected:
	shared_ptr<SubscriberStubImpl> _impl;
};


/**
 * Subscriber abstraction (bridge pattern).
 *
 * We need to overwrite everything to use the concrete implementors functions. The preferred
 * constructor is the Subscriber(string channelName, Receiver* receiver) one, the unqualified
 * constructor without a receiver and the setReceiver method are required for Java as we cannot
 * inherit publishers while being its receiver at the same time as is used for the TypedSubscriber.
 */
class DLLEXPORT Subscriber : public SubscriberStub, public ResultSet<PublisherStub> {
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

	std::set<string> getPublisherUUIDs()             {
		return _impl->getPublisherUUIDs();
	}
	bool isSubscribedTo(const string& uuid) {
		return _impl->getPublisherUUIDs().find(uuid) != _impl->getPublisherUUIDs().end();
	}

	void added(PublisherStub pub) {
		_impl->added(pub);
	}
	void changed(PublisherStub pub) {
		_impl->changed(pub);
	}
	void removed(PublisherStub pub) {
		_impl->removed(pub);
	}

	void suspend() {
		return _impl->suspend();
	}
	void resume() {
		return _impl->resume();
	}

	shared_ptr<SubscriberImpl> getImpl() const {
		return _impl;
	}

protected:

	shared_ptr<SubscriberImpl> _impl;
	shared_ptr<SubscriberConfig> _config;
	friend class Node;
};

}


#endif /* end of include guard: SUBSCRIBER_H_J64J09SP */
