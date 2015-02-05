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

#include "umundo/core/connection/Publisher.h"

#include "umundo/core/Factory.h"
#include "umundo/core/Message.h"
#include "umundo/core/UUID.h"

namespace umundo {

int PublisherImpl::instances = 0;

PublisherImpl::PublisherImpl() : _greeter(NULL) {
	instances++;
}

PublisherImpl::~PublisherImpl() {
	instances--;
}

Publisher::Publisher(const std::string& channelName) {
	PublisherConfigTCP config(channelName);
	init(&config);
}


Publisher::Publisher(PublisherConfig* config) {
	init(config);
}

void Publisher::init(PublisherConfig* config) {
	switch (config->_type) {
	case ZEROMQ:
		_impl = StaticPtrCast<PublisherImpl>(Factory::create("pub.zmq"));
		_impl->implType = ZEROMQ;
		break;
	case RTP:
		_impl = StaticPtrCast<PublisherImpl>(Factory::create("pub.rtp"));
		_impl->implType = RTP;
		break;

	default:
		break;
	}

	EndPoint::_impl = _impl;
	PublisherStub::_impl = _impl;

	_impl->setChannelName(config->_channelName);
	_impl->init(config);
}

void Publisher::send(const char* data, size_t length) {
	Message* msg = new Message(data, length);
	_impl->send(msg);
	delete(msg);
}

Publisher::~Publisher() {
}

}