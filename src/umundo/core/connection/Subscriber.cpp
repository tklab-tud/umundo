/**
 *  @file
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

#include "umundo/core/connection/Subscriber.h"
#include "umundo/core/Factory.h"
#include "umundo/core/UUID.h"

namespace umundo {

int SubscriberImpl::instances = -1;

SubscriberImpl::SubscriberImpl() : _receiver(NULL) {
	instances++;
}

SubscriberImpl::~SubscriberImpl() {
	instances--;
}

Subscriber::Subscriber(const std::string& channelName) {
	SubscriberConfig config(channelName);
	init(&config);
}

Subscriber::Subscriber(SubscriberConfig* config) {
	init(config);
}

Subscriber::~Subscriber() {
}

void Subscriber::init(SubscriberConfig* config) {
	switch (config->_type) {
	case RTP:
		_impl = StaticPtrCast<SubscriberImpl>(Factory::create("sub.rtp"));
		_impl->implType = RTP;
		break;
	case ZEROMQ:
		_impl = StaticPtrCast<SubscriberImpl>(Factory::create("sub.zmq"));
		_impl->implType = ZEROMQ;
		break;
	default:
		break;
	}

	EndPoint::_impl = _impl;
	SubscriberStub::_impl = _impl;
	_impl->setChannelName(config->_channelName);
	_impl->init(config);
}


}