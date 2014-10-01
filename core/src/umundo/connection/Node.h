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
#include "umundo/common/ResultSet.h"
#include "umundo/common/Implementation.h"
#include "umundo/connection/NodeStub.h"
#include "umundo/connection/Publisher.h"
#include "umundo/connection/Subscriber.h"

#include <boost/enable_shared_from_this.hpp>

#define SHORT_UUID(uuid) uuid.substr(0, 8)

namespace umundo {

class Connectable;
class Discovery;

class UMUNDO_API NodeOptions : public EndPointOptions {
public:

	NodeOptions(uint16_t nodePort, uint16_t pubPort) {
		setPort(nodePort);
		setPubPort(pubPort);
		options["node.allowLocal"] = toStr(false);
	}
	
	NodeOptions() {
		options["node.allowLocal"] = toStr(false);
	}
	
	NodeOptions(const std::string& address) : EndPointOptions(address) {
		setPubPort(strTo<uint16_t>(options["endpoint.port"]) + 1);
		options["node.allowLocal"] = toStr(false);
	}

	std::string getType() {
		return "NodeConfig";
	}
	
	void setPubPort(uint16_t pubPort) {
		options["node.port.pub"] = toStr(pubPort);
	}
		
	void allowLocalConnections(bool allow) {
		options["node.allowLocal"] = toStr(allow);
	}
};

/**
 * The local umundo node implementor basis class (bridge pattern).
 */
class UMUNDO_API NodeImpl : public NodeStubBaseImpl, public Implementation, public ResultSet<EndPoint> { //, public EnableSharedFromThis<NodeImpl> {
public:
	NodeImpl();
	virtual ~NodeImpl();

	virtual void addSubscriber(Subscriber& sub) = 0;
	virtual void removeSubscriber(Subscriber& sub) = 0;
	virtual void addPublisher(Publisher& pub) = 0;
	virtual void removePublisher(Publisher& pub) = 0;

	static int instances;

	virtual std::map<std::string, NodeStub> connectedFrom() = 0;
	virtual std::map<std::string, NodeStub> connectedTo() = 0;

	virtual std::map<std::string, Subscriber> getSubscribers() {
		return _subs;
	}

	virtual Subscriber getSubscriber(const std::string& uuid) {
		if (_subs.find(uuid) != _subs.end())
			return _subs[uuid];
		return nullSub;
	}

	virtual std::map<std::string, Publisher> getPublishers() {
		return _pubs;
	}

	virtual Publisher getPublisher(const std::string& uuid) {
		if (_pubs.find(uuid) != _pubs.end())
			return _pubs[uuid];
		return nullPub;
	}
	
	virtual void addPublisherMonitor(ResultSet<PublisherStub>* monitor) {
		_pubMonitors.push_back(monitor);
	}
	
	virtual void clearPublisherMonitors() {
		_pubMonitors.clear();
	}

protected:
	std::map<std::string, Publisher> _pubs;
	std::map<std::string, Subscriber> _subs;
	std::list<ResultSet<PublisherStub>* > _pubMonitors;

private:
	Publisher nullPub;
	Subscriber nullSub;
};

/**
 * The local umundo node abstraction (bridge pattern).
 */
class UMUNDO_API Node : public NodeStubBase {
public:

	Node();
	Node(const NodeOptions&);
	Node(uint16_t nodePort, uint16_t pubPort);
	Node(SharedPtr<NodeImpl> const impl) : NodeStubBase(impl), _impl(impl) { }
	Node(const Node& other) : NodeStubBase(other._impl), _impl(other._impl) { }
	virtual ~Node();

	operator bool() const {
		return _impl.get();
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

	void addPublisherMonitor(ResultSet<PublisherStub>* monitor) {
		_impl->addPublisherMonitor(monitor);
	}
	
	void clearPublisherMonitors() {
		_impl->clearPublisherMonitors();
	}
	
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

	std::map<std::string, NodeStub> connectedTo() {
		return _impl->connectedTo();
	}

	std::map<std::string, NodeStub> connectedFrom() {
		return _impl->connectedFrom();
	}

	virtual Subscriber getSubscriber(const std::string& uuid) {
		return _impl->getSubscriber(uuid);
	}

	virtual Publisher getPublisher(const std::string& uuid) {
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

	SharedPtr<NodeImpl> getImpl() const {
		return _impl;
	}

	virtual std::map<std::string, Subscriber> getSubscribers() {
		return _impl->getSubscribers();
	}

	virtual std::map<std::string, Publisher> getPublishers() {
		return _impl->getPublishers();
	}

	/** @name Remote endpoint awareness */
	//@{

	virtual void added(EndPoint endPoint) {
		return _impl->added(endPoint);
	}
	virtual void removed(EndPoint endPoint) {
		return _impl->removed(endPoint);
	}
	virtual void changed(EndPoint endPoint) {
		return _impl->removed(endPoint);
	}

	//@}

protected:
	void init();

	SharedPtr<NodeImpl> _impl;

	friend class Discovery;
};

/**
 * Convinience class to register an entity with many publishers / subscribers.
 */
class UMUNDO_API Connectable {
public:
	virtual ~Connectable() {};
	// namepace qualifiers are required for swig typemaps!
	virtual std::map<std::string, umundo::Publisher> getPublishers() {
		return std::map<std::string, umundo::Publisher>();
	}
	virtual std::map<std::string, umundo::Subscriber> getSubscribers() {
		return std::map<std::string, Subscriber>();
	}

	// notify connectable that it has been added to a node
	virtual void addedToNode(Node& node) {
	}

	// notify connectable that it has been removed from a node
	virtual void removedFromNode(Node& node) {
	}
};


}


#endif /* end of include guard: NODE_H_AA94X8L6 */
