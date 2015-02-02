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

#ifndef NODESTUB_H_9BQGCEIW
#define NODESTUB_H_9BQGCEIW

#include "umundo/core/Common.h"
#include "umundo/core/EndPoint.h"
#include "umundo/core/ResultSet.h"
#include "umundo/core/Implementation.h"
#include "umundo/core/connection/PublisherStub.h"
#include "umundo/core/connection/SubscriberStub.h"

#include <boost/enable_shared_from_this.hpp>

namespace umundo {

class Connectable;
class Discovery;


class UMUNDO_API NodeStubImpl : public EndPointImpl {
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

class UMUNDO_API NodeStub : public EndPoint {
public:
	NodeStub() : _impl() { }
	NodeStub(SharedPtr<NodeStubImpl> const impl) : EndPoint(impl), _impl(impl) { }
	NodeStub(const NodeStub& other) : EndPoint(other._impl), _impl(other._impl) { }
	virtual ~NodeStub() { }

	operator bool() const {
		return _impl.get();
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

	SharedPtr<NodeStubImpl> getImpl() const {
		return _impl;
	}

	virtual std::map<std::string, SubscriberStub> getSubscribers() {
		return _impl->getSubscribers();
	}

	virtual std::map<std::string, PublisherStub> getPublishers() {
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
	SharedPtr<NodeStubImpl> _impl;

};


}


#endif /* end of include guard: NODESTUB_H_9BQGCEIW */
