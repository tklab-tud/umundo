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

#ifndef SUBSCRIBERSTUB_H_YXXKE6LO
#define SUBSCRIBERSTUB_H_YXXKE6LO

#include "umundo/core/Common.h"
#include "umundo/core/UUID.h"
#include "umundo/core/EndPoint.h"
#include "umundo/core/Implementation.h"

#include <list>

namespace umundo {

class NodeStub;
class Message;

class UMUNDO_API SubscriberStubImpl : public EndPointImpl {
public:
	SubscriberStubImpl() : _multicast(false) {
		_uuid = UUID::getUUID();
	}
	virtual ~SubscriberStubImpl() {}
	virtual std::string getChannelName() const            {
		return _channelName;
	}
	virtual void setChannelName(const std::string& channelName) {
		_channelName = channelName;
	}

	virtual bool isMulticast() const                 {
		return _multicast;
	}
	virtual void setMulticast(const bool multicast)  {
		_multicast = multicast;
	}

protected:
	std::string _channelName;
	bool _multicast;
};

/**
 * Subscriber implementor basis class (bridge pattern).
 */
class UMUNDO_API SubscriberStub : public EndPoint {
public:
	enum SubscriberType {
		// these have to fit the publisher types!
		ZEROMQ = 0x0001,
		RTP    = 0x0002
	};

	virtual ~SubscriberStub() {}
	SubscriberStub() : _impl() { }
	SubscriberStub(SharedPtr<SubscriberStubImpl> const impl) : EndPoint(impl), _impl(impl) { }
	SubscriberStub(const SubscriberStub& other) : EndPoint(other._impl), _impl(other._impl) { }

	operator bool() const {
		return _impl.get();
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
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	virtual const std::string getChannelName() const     {
		return _impl->getChannelName();
	}
	virtual const bool isRTP() const                     {
		return _impl->implType == RTP;
	}
	virtual const bool isMulticast() const               {
		return _impl->isMulticast();
	}

	SharedPtr<SubscriberStubImpl> getImpl() const {
		return _impl;
	}

protected:
	SharedPtr<SubscriberStubImpl> _impl;
};

}

#endif /* end of include guard: SUBSCRIBERSTUB_H_YXXKE6LO */
