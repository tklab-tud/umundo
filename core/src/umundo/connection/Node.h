/**
 *  @file
 *  @brief      Classes and interfaces to connect Publisher%s and Subscriber%s.
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

#ifndef NODE_H_AA94X8L6
#define NODE_H_AA94X8L6

#include "umundo/common/Common.h"
#include "umundo/common/EndPoint.h"
#include "umundo/common/Implementation.h"
#include "umundo/connection/Publisher.h"
#include "umundo/connection/Subscriber.h"

#include <boost/enable_shared_from_this.hpp>

#define SHORT_UUID(uuid) uuid.substr(0, 8)

namespace umundo {

class Connectable;

class DLLEXPORT NodeStubBaseImpl : public EndPointImpl {
public:
	std::string getUUID() const {
		return _uuid;
	}
	virtual void setUUID(const std::string& uuid) {
		_uuid = uuid;
	}

protected:
	std::string _uuid;

};

/**
 * Representation of a remote umundo Node.
 */
class DLLEXPORT NodeStubBase : public EndPoint {
public:
	NodeStubBase() : _impl() { }
	NodeStubBase(boost::shared_ptr<NodeStubBaseImpl> const impl) : EndPoint(impl), _impl(impl) { }
	NodeStubBase(const NodeStubBase& other) : EndPoint(other._impl), _impl(other._impl) { }
	virtual ~NodeStubBase() { }

	operator bool() const {
		return _impl;
	}
	bool operator< (const NodeStubBase& other) const {
		return _impl < other._impl;
	}
	bool operator==(const NodeStubBase& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const NodeStubBase& other) const {
		return _impl != other._impl;
	}

	NodeStubBase& operator=(const NodeStubBase& other) {
		_impl = other._impl;
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	/** @name Remote Node */
	//@{
	virtual const string getUUID() const {
		return _impl->getUUID();
	}

	//@}

	boost::shared_ptr<NodeStubBaseImpl> getImpl() const {
		return _impl;
	}

protected:
	boost::shared_ptr<NodeStubBaseImpl> _impl;
};

class DLLEXPORT NodeStubImpl : public NodeStubBaseImpl {
public:
	virtual void addPublisher(const PublisherStub& pub) {
		_pubs[pub.getUUID()] = pub;
	}
	virtual void removePublisher(const PublisherStub& pub) {
		if (_pubs.find(pub.getUUID()) != _pubs.end()) {
			_pubs.erase(pub.getUUID());
		}
	}
	virtual void addSubscriber(const SubscriberStub& sub) {
		_subs[sub.getUUID()] = sub;
	}
	virtual void removeSubscriber(const SubscriberStub& sub) {
		if (_subs.find(sub.getUUID()) != _subs.end()) {
			_subs.erase(sub.getUUID());
		}
	}

	virtual std::map<std::string, SubscriberStub>& getSubscribers() {
		return _subs;
	}

	virtual SubscriberStub& getSubscriber(const std::string& uuid) {
		if (_subs.find(uuid) != _subs.end())
			return _subs[uuid];
		return nullSubStub;
	}

	virtual std::map<std::string, PublisherStub>& getPublishers() {
		return _pubs;
	}

	virtual PublisherStub& getPublisher(const std::string& uuid) {
		if (_pubs.find(uuid) != _pubs.end())
			return _pubs[uuid];
		return nullPubStub;
	}

private:
	PublisherStub nullPubStub;
	SubscriberStub nullSubStub;
	std::map<std::string, PublisherStub> _pubs;
	std::map<std::string, SubscriberStub> _subs;

};

class DLLEXPORT NodeStub : public NodeStubBase {
public:
	NodeStub() : _impl() { }
	NodeStub(boost::shared_ptr<NodeStubImpl> const impl) : NodeStubBase(impl), _impl(impl) { }
	NodeStub(const NodeStub& other) : NodeStubBase(other._impl), _impl(other._impl) { }
	virtual ~NodeStub() { }

	operator bool() const {
		return _impl;
	}
	bool operator< (const NodeStub& other) const {
		return _impl < other._impl;
	}
	bool operator==(const NodeStub& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const NodeStub& other) const {
		return _impl != other._impl;
	}

	NodeStub& operator=(const NodeStub& other) {
		_impl = other._impl;
		EndPoint::_impl = _impl;
		NodeStubBase::_impl = _impl;
		return *this;
	} // operator=

	virtual void addPublisher(const PublisherStub& pub) {
		return _impl->addPublisher(pub);
	}
	virtual void removePublisher(const PublisherStub& pub) {
		return _impl->removePublisher(pub);
	}
	virtual void addSubscriber(const SubscriberStub& sub) {
		return _impl->addSubscriber(sub);
	}
	virtual void removeSubscriber(const SubscriberStub& sub) {
		return _impl->removeSubscriber(sub);
	}

	virtual SubscriberStub& getSubscriber(const std::string& uuid) const {
		return _impl->getSubscriber(uuid);
	}

	virtual PublisherStub& getPublisher(const std::string& uuid) const   {
		return _impl->getPublisher(uuid);
	}

	boost::shared_ptr<NodeStubImpl> getImpl() const {
		return _impl;
	}

	virtual std::map<std::string, SubscriberStub>& getSubscribers() {
		return _impl->getSubscribers();
	}
	
	virtual std::map<std::string, PublisherStub>& getPublishers() {
		return _impl->getPublishers();
	}

#if 0
	virtual std::set<SubscriberStub> getSubscribers() {
		std::map<std::string, SubscriberStub> subs = _impl->getSubscribers();
		std::set<SubscriberStub> subSet;
		for( std::map<std::string, SubscriberStub>::iterator it = subs.begin(); it != subs.end(); ++it ) {
			subSet.insert(it->second);
		}
		return subSet;
	}

	virtual std::set<PublisherStub> getPublishers() {
		std::map<std::string, PublisherStub> pubs = _impl->getPublishers();
		std::set<PublisherStub> pubSet;
		for( std::map<std::string, PublisherStub>::iterator it = pubs.begin(); it != pubs.end(); ++it ) {
			pubSet.insert(it->second);
		}
		return pubSet;
	}
#endif

protected:
	boost::shared_ptr<NodeStubImpl> _impl;

};

/**
 * The local umundo node implementor basis class (bridge pattern).
 */
class DLLEXPORT NodeImpl : public NodeStubBaseImpl, public Implementation, public boost::enable_shared_from_this<NodeImpl> {
public:
	NodeImpl();
	virtual ~NodeImpl();

	virtual void addSubscriber(Subscriber& sub) = 0;
	virtual void removeSubscriber(Subscriber& sub) = 0;
	virtual void addPublisher(Publisher& pub) = 0;
	virtual void removePublisher(Publisher& pub) = 0;

	static int instances;

	virtual std::map<std::string, Subscriber>& getSubscribers() {
		return _subs;
	}

	virtual Subscriber& getSubscriber(const std::string& uuid) {
		if (_subs.find(uuid) != _subs.end())
			return _subs[uuid];
		return nullSub;
	}

	virtual std::map<std::string, Publisher>& getPublishers() {
		return _pubs;
	}

	virtual Publisher& getPublisher(const std::string& uuid) {
		if (_pubs.find(uuid) != _pubs.end())
			return _pubs[uuid];
		return nullPub;
	}


protected:
	std::map<std::string, Publisher> _pubs;
	std::map<std::string, Subscriber> _subs;

private:
	Publisher nullPub;
	Subscriber nullSub;
};

/**
 * The local umundo node abstraction (bridge pattern).
 */
class DLLEXPORT Node : public NodeStubBase {
public:

	Node();
	Node(const std::string domain);
	Node(boost::shared_ptr<NodeImpl> const impl) : NodeStubBase(impl), _impl(impl) { }
	Node(const Node& other) : NodeStubBase(other._impl), _impl(other._impl) { }
	virtual ~Node();

	operator bool() const {
		return _impl;
	}
	bool operator< (const Node& other) const {
		return _impl < other._impl;
	}
	bool operator==(const Node& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const Node& other) const {
		return _impl != other._impl;
	}

	Node& operator=(const Node& other) {
		_impl = other._impl;
		EndPoint::_impl = _impl;
		NodeStubBase::_impl = _impl;
		return *this;
	} // operator=

	/** @name Publish / Subscriber Maintenance */
	//@{

	void addSubscriber(Subscriber sub) {
		return _impl->addSubscriber(sub);
	}
	void removeSubscriber(Subscriber sub) {
		return _impl->removeSubscriber(sub);
	}
	void addPublisher(Publisher pub) {
		return _impl->addPublisher(pub);
	}
	void removePublisher(Publisher pub) {
		return _impl->removePublisher(pub);
	}

	virtual Subscriber& getSubscriber(const std::string& uuid) {
		return _impl->getSubscriber(uuid);
	}
	
	virtual Publisher& getPublisher(const std::string& uuid) {
		return _impl->getPublisher(uuid);
	}
	
	void connect(Connectable* connectable);
	void disconnect(Connectable* connectable);
	
	//@}
	
	void suspend() {
		return _impl->suspend();
	}
	void resume() {
		return _impl->resume();
	}
	
	shared_ptr<NodeImpl> getImpl() const {
		return _impl;
	}

	virtual std::map<std::string, Subscriber>& getSubscribers() {
		return _impl->getSubscribers();
	}

	virtual std::map<std::string, Publisher>& getPublishers() {
		return _impl->getPublishers();
	}

#if 0
	virtual std::set<Subscriber> getSubscribers() {
		std::map<std::string, Subscriber> subs = _impl->getSubscribers();
		std::set<Subscriber> subSet;
		for( std::map<std::string, Subscriber>::iterator it = subs.begin(); it != subs.end(); ++it ) {
			subSet.insert(it->second);
		}
		return subSet;
	}
	
	virtual std::set<Publisher> getPublishers() {
		std::map<std::string, Publisher> pubs = _impl->getPublishers();
		std::set<Publisher> pubSet;
		for( std::map<std::string, Publisher>::iterator it = pubs.begin(); it != pubs.end(); ++it ) {
			pubSet.insert(it->second);
		}
		return pubSet;
	}
#endif

protected:
	boost::shared_ptr<NodeImpl> _impl;

	friend class Discovery;
};

/**
 * Convinience class to register an entity with many publishers / subscribers.
 */
class DLLEXPORT Connectable {
public:
	virtual ~Connectable() {};
	// namepace qualifiers are required for swig typemaps!
	virtual std::set<umundo::Publisher> getPublishers() {
		return set<Publisher>();
	}
	virtual std::set<umundo::Subscriber> getSubscribers() {
		return set<Subscriber>();
	}

	// notify connectable that it has been added to a node
	virtual void addedToNode(Node& node) {
	}

	// notify connectable that it has been removed from a node
	virtual void removedFromNode(Node& node) {
	}
};

class DLLEXPORT NodeConfig : public Configuration {
public:
	shared_ptr<Configuration> create();
	virtual ~NodeConfig() {}

	string domain;
	string transport;
	string host;
	uint16_t port;
	string uuid;
};


}


#endif /* end of include guard: NODE_H_AA94X8L6 */
