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

#ifndef PUBLISHERSTUB_H_7YEPL1XG
#define PUBLISHERSTUB_H_7YEPL1XG

#include "umundo/core/Common.h"
#include "umundo/core/UUID.h"
#include "umundo/core/EndPoint.h"
#include "umundo/core/Implementation.h"

namespace umundo {

class Message;
class Publisher;
class SubscriberStub;

class PublisherStubImpl : public EndPointImpl {
public:
	PublisherStubImpl() {
		_uuid = UUID::getUUID();
	}
	virtual std::string getChannelName() const            {
		return _channelName;
	}
	virtual void setChannelName(const std::string& channelName) {
		_channelName = channelName;
	}

protected:
	std::string _channelName;
};

/**
 * Representation of a remote Publisher.
 */
class UMUNDO_API PublisherStub : public EndPoint {
public:
	enum PublisherType {
		// these have to fit the subscriber types!
		ZEROMQ = 0x0001,
		RTP    = 0x0002
	};

	PublisherStub() : _impl() { }
	PublisherStub(SharedPtr<PublisherStubImpl> const impl) : EndPoint(impl), _impl(impl) { }
	PublisherStub(const PublisherStub& other) : EndPoint(other._impl), _impl(other._impl) { }

	operator bool() const {
		return _impl.get();
	}
	bool operator< (const PublisherStub& other) const {
		return _impl < other._impl;
	}
	bool operator==(const PublisherStub& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const PublisherStub& other) const {
		return _impl != other._impl;
	}

	PublisherStub& operator=(const PublisherStub& other) {
		_impl = other._impl;
		EndPoint::_impl = _impl;
		return *this;
	} // operator=

	SharedPtr<PublisherStubImpl> getImpl() const {
		return _impl;
	}

	/** @name Functionality of local *and* remote Publishers */
	//@{
	virtual const std::string getChannelName() const      {
		return _impl->getChannelName();
	}
	virtual const bool isRTP() const                           {
		return _impl->implType == RTP;
	}
	//@}

protected:
	SharedPtr<PublisherStubImpl> _impl;
};

#if 0
std::ostream & operator<<(std::ostream &os, const PublisherStub& pubStub) {
	os << "PublisherStub:" << std::endl;
	os << "\tchannelName: " << pubStub.getChannelName() << std::endl;
	os << "\tuuid: " << pubStub.getUUID() << std::endl;
	os << static_cast<EndPoint>(pubStub);
	return os;
}
#endif

}

#endif /* end of include guard: PUBLISHERSTUB_H_7YEPL1XG */
