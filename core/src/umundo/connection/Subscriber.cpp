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

#include "umundo/connection/Subscriber.h"
#include "umundo/common/Factory.h"
#include "umundo/common/UUID.h"

namespace umundo {

int SubscriberImpl::instances = -1;

SubscriberImpl::SubscriberImpl() : _receiver(NULL) {
	instances++;
}

SubscriberImpl::~SubscriberImpl() {
	instances--;
}

Subscriber::Subscriber(SubscriberType type, const std::string& channelName) {
	SubscriberConfig config;
	init(type, channelName, NULL, &config);
}

Subscriber::Subscriber(SubscriberType type, const std::string& channelName, Receiver* receiver) {
	SubscriberConfig config;
	init(type, channelName, receiver, &config);
}

Subscriber::Subscriber(SubscriberType type, const std::string& channelName, SubscriberConfig* config) {
	init(type, channelName, NULL, config);
}

Subscriber::Subscriber(SubscriberType type, const std::string& channelName, Receiver* receiver, SubscriberConfig* config) {
	init(type, channelName, receiver, config);
}

Subscriber::Subscriber(const std::string& channelName) {
	SubscriberConfig config;
	init(ZEROMQ, channelName, NULL, &config);
}

Subscriber::Subscriber(const std::string& channelName, Receiver* receiver) {
	SubscriberConfig config;
	init(ZEROMQ, channelName, receiver, &config);
}

Subscriber::Subscriber(const std::string& channelName, SubscriberConfig* config) {
	init(ZEROMQ, channelName, NULL, config);
}

Subscriber::Subscriber(const std::string& channelName, Receiver* receiver, SubscriberConfig* config) {
	init(ZEROMQ, channelName, receiver, config);
}

Subscriber::~Subscriber() {
}

void Subscriber::init(SubscriberType type, const std::string& channelName, Receiver* receiver, SubscriberConfig* config) {
	switch (type) {
	case RTP:
		_impl = boost::static_pointer_cast<SubscriberImpl>(Factory::create("sub.rtp"));
		_impl->implType = RTP;
		break;
	case ZEROMQ:
		_impl = boost::static_pointer_cast<SubscriberImpl>(Factory::create("sub.zmq"));
		_impl->implType = ZEROMQ;
		break;
	default:
		break;
	}

	SubscriberStub::_impl = _impl;
	_impl->setChannelName(channelName);
	_impl->init(config);
	if (receiver != NULL)
		_impl->setReceiver(receiver);
}


}