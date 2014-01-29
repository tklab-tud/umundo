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
#include "umundo/connection/PublisherStub.h"
#include "umundo/connection/SubscriberStub.h"
#include "umundo/connection/NodeStub.h"
#include "umundo/common/EndPoint.h"
#include "umundo/common/Implementation.h"

namespace umundo {

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
	virtual void welcome(const Publisher&, const SubscriberStub&) = 0;
	virtual void farewell(const Publisher&, const SubscriberStub&) = 0;
};

class DLLEXPORT PublisherConfig : public Options {
public:
	virtual std::string getType() {
		return "PublisherConfig";
	}

	std::string channelName;
};

class DLLEXPORT RTPPublisherConfig : public PublisherConfig {
public:
	std::string getType() {
		return "RTPPublisherConfig";
	}

	RTPPublisherConfig(uint32_t increment, int type=-1, uint16_t port=0) {
		setTimestampIncrement(increment);
		if(type>0 && type<128)		//libre limit
			setPayloadType(type);
		if(port)
			setPortbase(port);
	}

	void setTimestampIncrement(uint32_t increment) {
		options["pub.rtp.timestampIncrement"] = toStr(increment);
	}

	void setPortbase(uint16_t port) {
		options["pub.rtp.portbase"] = toStr(port);
	}

	void setPayloadType(uint8_t type) {
		options["pub.rtp.payloadType"] = toStr(type);
	}

	void setMulticastIP(std::string ip) {
		options["pub.rtp.multicast"] = ip;
	}

	void setMulticastPortbase(uint16_t port) {
		options["pub.rtp.multicastPortbase"] = toStr(port);
	}
};

/**
 * Publisher implementor basis class (bridge pattern)
 */
class DLLEXPORT PublisherImpl : public PublisherStubImpl, public Implementation {
public:
	PublisherImpl();
	virtual ~PublisherImpl();

	virtual void send(Message* msg) = 0;
	void putMeta(const std::string& key, const std::string& value) {
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
	std::map<std::string, SubscriberStub> getSubscribers() {
		return _subs;
	}
	bool isPublishingTo(const std::string& subUUID) {
		return _subs.find(subUUID) != _subs.end();
	}

	//@}

	static int instances;

protected:
	/** @name Subscriber awareness */
	//@{
	virtual void added(const SubscriberStub& sub, const NodeStub& node) = 0;
	virtual void removed(const SubscriberStub& sub, const NodeStub& node) = 0;
	//@}

	std::map<std::string, std::string> _mandatoryMeta;
	std::map<std::string, SubscriberStub> _subs;

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
	enum PublisherType {
	    // these have to fit the subscriber types!
	    ZEROMQ = 0x0001,
	    RTP    = 0x0002
	};

	Publisher() : _impl() {}
	Publisher(const std::string& channelName);
	Publisher(const std::string& channelName, Greeter* greeter);
	Publisher(const std::string& channelName, PublisherConfig* config);
	Publisher(const std::string& channelName, Greeter* greeter, PublisherConfig* config);
	Publisher(PublisherType type, const std::string& channelName);
	Publisher(PublisherType type, const std::string& channelName, Greeter* greeter);
	Publisher(PublisherType type, const std::string& channelName, PublisherConfig* config);
	Publisher(PublisherType type, const std::string& channelName, Greeter* greeter, PublisherConfig* config);
	Publisher(const boost::shared_ptr<PublisherImpl> impl) : PublisherStub(impl), _impl(impl) { }
	Publisher(const Publisher& other) : PublisherStub(other._impl), _impl(other._impl) { }
	virtual ~Publisher();

	operator bool() const {
		return _impl.get();
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
	void putMeta(const std::string& key, const std::string& value) {
		return _impl->putMeta(key, value);
	}
	//@}

	bool isPublishingTo(const std::string& subUUID) {
		return _impl->isPublishingTo(subUUID);
	}

	std::map<std::string, SubscriberStub> getSubscribers() {
		return _impl->getSubscribers();
	}

	void suspend() {
		return _impl->suspend();
	}
	void resume() {
		return _impl->resume();
	}

	void added(const SubscriberStub& sub, const NodeStub& node)   {
		_impl->added(sub, node);
	}
	void removed(const SubscriberStub& sub, const NodeStub& node) {
		_impl->removed(sub, node);
	}

protected:
	void init(PublisherType type, const std::string& channelName, Greeter* greeter, PublisherConfig* config);

	boost::shared_ptr<PublisherImpl> _impl;
	friend class Node;
};

}


#endif /* end of include guard: PUBLISHER_H_F3M1RWLN */
