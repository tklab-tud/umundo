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

#include "umundo/core/Common.h"
#include "umundo/core/UUID.h"
#include "umundo/core/connection/Publisher.h"
#include "umundo/core/connection/SubscriberStub.h"
#include "umundo/core/EndPoint.h"
#include "umundo/core/Implementation.h"

#include <list>

namespace umundo {

class NodeStub;
class Message;
class Publisher;
class PublisherStub;
class SubscriberConfig;

/**
 * Interface for client classes to get byte-arrays from subscribers.
 */
class UMUNDO_API Receiver {
public:
	virtual ~Receiver() {}
	virtual void receive(Message* msg) = 0;
	friend class Subscriber;
};


/**
 * Subscriber implementor basis class (bridge pattern).
 */
class UMUNDO_API SubscriberImpl : public SubscriberStubImpl, public Implementation {
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

	virtual bool matches(const PublisherStub& pub) {
		// are our types equal and is our channel a prefix of the given channel?
		return (pub.getImpl()->implType == implType &&
		        pub.getChannelName().substr(0, _channelName.size()) == _channelName);
	}

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
class UMUNDO_API Subscriber : public SubscriberStub {
public:
	Subscriber() : _impl() {}
	Subscriber(const std::string& channelName);
	Subscriber(SubscriberConfig* config);

	explicit Subscriber(const SubscriberStub& stub) : _impl(StaticPtrCast<SubscriberImpl>(stub.getImpl())) {}
	Subscriber(SharedPtr<SubscriberImpl> const impl) : SubscriberStub(impl), _impl(impl) { }
	Subscriber(const Subscriber& other) : SubscriberStub(other._impl), _impl(other._impl) { }
	virtual ~Subscriber();

	operator bool() const {
		return _impl.get();
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
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	void setReceiver(Receiver* receiver) {
		_impl->setReceiver(receiver);
	}

	Receiver* getReceiver() {
		return _impl->getReceiver();
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

	std::map<std::string, PublisherStub> getPublishers()             {
		return _impl->getPublishers();
	}
	bool isSubscribedTo(const std::string& uuid) {
		std::map<std::string, PublisherStub> pubs = _impl->getPublishers();
		return pubs.find(uuid) != pubs.end();
	}

	bool matches(const PublisherStub& pub) {
		return _impl->matches(pub);
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

	SharedPtr<SubscriberImpl> getImpl() const {
		return _impl;
	}

protected:

	void init(SubscriberConfig* config);

	SharedPtr<SubscriberImpl> _impl;
	friend class Node;
};


class UMUNDO_API SubscriberConfig : public Options {
protected:
	SubscriberConfig(const std::string& channel) : _channelName(channel) {
		_type = Subscriber::ZEROMQ;
	}

	std::string _channelName;
	SubscriberStub::SubscriberType _type;

	friend class Subscriber;
};


class UMUNDO_API SubscriberConfigTCP : public SubscriberConfig {
public:
	SubscriberConfigTCP(const std::string& channel) : SubscriberConfig(channel) {
		_type = Subscriber::ZEROMQ;
	}

protected:
	friend class Subscriber;
};


class UMUNDO_API SubscriberConfigRTP : public SubscriberConfigTCP {
public:
	SubscriberConfigRTP(const std::string& channel) : SubscriberConfigTCP(channel) {
		_type = Subscriber::RTP;
	}

	void setPortbase(uint16_t port) {
		options["sub.rtp.portbase"] = toStr(port);
	}

	void setMulticastIP(std::string ip) {
		options["sub.rtp.multicast"] = ip;
	}

	void setMulticastPortbase(uint16_t port) {
		if(options.find("sub.rtp.multicast") == options.end())
			setMulticastIP("239.8.4.8");		//default multicast address for umundo rtp
		setPortbase(port);
	}
};


class UMUNDO_API SubscriberConfigMCast : public SubscriberConfigRTP {
public:
	SubscriberConfigMCast(const std::string& channel) : SubscriberConfigRTP(channel) {}
};

}


#endif /* end of include guard: SUBSCRIBER_H_J64J09SP */
