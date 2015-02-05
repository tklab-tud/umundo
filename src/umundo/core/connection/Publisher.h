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

#include "umundo/core/Common.h"
#include "umundo/core/UUID.h"
#include "umundo/core/connection/PublisherStub.h"
#include "umundo/core/connection/SubscriberStub.h"
#include "umundo/core/connection/NodeStub.h"
#include "umundo/core/EndPoint.h"
#include "umundo/core/Implementation.h"

namespace umundo {

class Message;
class Publisher;
class PublisherConfig;

/**
 * Wait for new subscribers and welcome them.
 *
 * A greeter can be registered at a publisher to send a message, whenever a new subscriber
 * is added to the publisher.
 */
class UMUNDO_API Greeter {
public:
	virtual ~Greeter() {};
	virtual void welcome(Publisher&, const SubscriberStub&) = 0;
	virtual void farewell(Publisher&, const SubscriberStub&) = 0;
};

/**
 * Publisher implementor basis class (bridge pattern)
 */
class UMUNDO_API PublisherImpl : public PublisherStubImpl, public Implementation {
public:
	PublisherImpl();
	virtual ~PublisherImpl();

	virtual void send(Message* msg) = 0;

	/** Meta fields to be set on every message published */
	void putMeta(const std::string& key, const std::string& value) {
		_mandatoryMeta[key] = value;
	}
	void clearMeta(const std::string& key) {
		_mandatoryMeta.erase(key);
	}

	/** @name Optional subscriber awareness */
	//@{
	virtual int waitForSubscribers(int count, int timeoutMs)        {
		return -1;
	}
	virtual void setGreeter(Greeter* greeter)        {
		_greeter = greeter;
	}
	virtual Greeter* getGreeter()        {
		return _greeter;
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
class UMUNDO_API Publisher : public PublisherStub {
public:

	Publisher() : _impl() {}
	Publisher(const std::string& channelName); //< Default publisher via TCP
	Publisher(PublisherConfig* config); //< Pass any subtype of PublisherConfig

	Publisher(const SharedPtr<PublisherImpl> impl) : PublisherStub(impl), _impl(impl) { }
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
	Greeter* getGreeter()                    {
		return _impl->getGreeter();
	}
	void putMeta(const std::string& key, const std::string& value) {
		return _impl->putMeta(key, value);
	}
	void clearMeta(const std::string& key) {
		return _impl->clearMeta(key);
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
	void init(PublisherConfig* config);

	SharedPtr<PublisherImpl> _impl;
	friend class Node;
};


class UMUNDO_API PublisherConfig : public Options {
protected:
	PublisherConfig(const std::string& channel) : _channelName(channel) {}
	std::string _channelName;
	PublisherStub::PublisherType _type;
	friend class Publisher;
};


class UMUNDO_API PublisherConfigTCP : public PublisherConfig {
public:
	PublisherConfigTCP(const std::string& channel) : PublisherConfig(channel) {
		_type = Publisher::ZEROMQ;
	}

	void enableCompression() {
		options["pub.tcp.compression"] = "1";
	}

protected:
	friend class Publisher;
};


class UMUNDO_API PublisherConfigRTP : public PublisherConfig {
public:
	PublisherConfigRTP(const std::string& channel) : PublisherConfig(channel) {
		_type = Publisher::RTP;
		setTimestampIncrement(150);
	}

	void setTimestampIncrement(uint32_t increment) {
		options["pub.rtp.timestampIncrement"] = toStr(increment);
	}

	void setPortbase(uint16_t port) {
		options["pub.rtp.portbase"] = toStr(port);
	}

	void setPayloadType(uint8_t type) {
		if(type>0 && type<128) {		//libre limit
			options["pub.rtp.payloadType"] = toStr(type);
		} else {
			UM_LOG_ERR("Publisher::Config::RTP:setPayloadType() error: ignoring invalid payload type");
		}
	}
};


class UMUNDO_API PublisherConfigMCast : public PublisherConfigRTP {
public:
	PublisherConfigMCast(const std::string& channel) : PublisherConfigRTP(channel) {}
};

}


#endif /* end of include guard: PUBLISHER_H_F3M1RWLN */
