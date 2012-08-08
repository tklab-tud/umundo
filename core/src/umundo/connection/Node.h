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

#define SHORT_UUID(uuid) uuid.substr(0, 8)

namespace umundo {

class Publisher;
class PublisherImpl;
class PublisherStub;
class Subscriber;
class SubscriberImpl;
class SubscriberStub;
class NodeStub;
class Node;
class Connectable;

/**
 * Convinience class to connect register an entity with many publishers / subscribers.
 */
class DLLEXPORT Connectable {
public:
	virtual ~Connectable() {};
	// namepace qualifiers are required for swig typemaps!
	virtual std::set<umundo::Publisher*> getPublishers() {
		return set<Publisher*>();
	}
	virtual std::set<umundo::Subscriber*> getSubscribers() {
		return set<Subscriber*>();
	}

	// notify connectable that it has been added to a node
	virtual void addedToNode(Node* node) {
	}

	// notify connectable that it has been removed from a node
	virtual void removedFromNode(Node* node) {
	}
};

/**
 * Representation of a remote umundo Node.
 */
class DLLEXPORT NodeStub : public EndPoint {
public:
	virtual ~NodeStub() {}

	/** @name Remote Node */
	//@{
	virtual const string& getUUID() const       {
		return _uuid;
	}
	virtual void setUUID(string uuid)           {
		_uuid = uuid;
	}

	//@}

	inline bool operator==(NodeStub* n) const {
		return (getUUID().compare(n->getUUID()) == 0);
	}

	inline bool operator!=(NodeStub* n) const {
		return (getUUID().compare(n->getUUID()) != 0);
	}

	/** @name Publish / Subscriber Maintenance */
	//@{
#if !defined(SWIGJAVA) && !defined(SWIGCSHARP)
	virtual void addSubscriber(shared_ptr<SubscriberStub>);
	virtual void removeSubscriber(shared_ptr<SubscriberStub>);
	virtual void addPublisher(shared_ptr<PublisherStub>);
	virtual void removePublisher(shared_ptr<PublisherStub>);

  bool hasSubscriber(const string& uuid);
  shared_ptr<SubscriberStub> getSubscriber(const string& uuid);
  map<string, shared_ptr<SubscriberStub> >& getSubscribers() { return _subs; }

  bool hasPublisher(const string& uuid);
  shared_ptr<PublisherStub> getPublisher(const string& uuid);
  map<string, shared_ptr<PublisherStub> >& getPublishers() { return _pubs; }
#endif
	//@}

protected:
	string _uuid;
	map<string, shared_ptr<PublisherStub> > _pubs; ///< UUIDS to publishers.
	map<string, shared_ptr<SubscriberStub> > _subs; ///< UUIDS to subscribers.

private:
	friend std::ostream& operator<<(std::ostream&, const NodeStub*);
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

class Node;
/**
 * The local umundo node implementor basis class (bridge pattern).
 */
class DLLEXPORT NodeImpl : public Implementation, public NodeStub {
public:
	NodeImpl();
	virtual ~NodeImpl() {}

#if !defined(SWIGJAVA) && !defined(SWIGCSHARP)
	virtual void addSubscriber(shared_ptr<SubscriberImpl>) = 0;
	virtual void removeSubscriber(shared_ptr<SubscriberImpl>) = 0;
	virtual void addPublisher(shared_ptr<PublisherImpl>) = 0;
	virtual void removePublisher(shared_ptr<PublisherImpl>) = 0;

  using NodeStub::addSubscriber;
  using NodeStub::removeSubscriber;
  using NodeStub::addPublisher;
  using NodeStub::removePublisher;
#endif  
};

/**
 * The local umundo node abstraction (bridge pattern).
 */
class DLLEXPORT Node : public virtual NodeStub {
public:
	Node();
	Node(string domain);
	virtual ~Node();

	/** @name Publish / Subscriber Maintenance */
	//@{
	void addSubscriber(Subscriber*);
	void removeSubscriber(Subscriber*);
	void addPublisher(Publisher*);
	void removePublisher(Publisher*);
	void connect(Connectable*);
	void disconnect(Connectable*);

#if !defined(SWIGJAVA) && !defined(SWIGCSHARP)
  using NodeStub::addSubscriber;
  using NodeStub::removeSubscriber;
  using NodeStub::addPublisher;
  using NodeStub::removePublisher;
#endif
	//@}

	/** @name Sleeping for Power Saving */
	//@{
	void suspend();
	void resume();
	//@}

	/** @name EndPoint */
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
	//@{

	/** @name Node Stub */
	virtual const string& getUUID() const       {
		return _impl->getUUID();
	}
	virtual void setUUID(string uuid)           {
		_impl->setUUID(uuid);
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

	static int instances;

protected:
	shared_ptr<NodeImpl> _impl;
	shared_ptr<NodeConfig> _config;

	friend class Discovery;
};

}


#endif /* end of include guard: NODE_H_AA94X8L6 */
