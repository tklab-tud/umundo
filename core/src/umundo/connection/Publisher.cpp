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

#include "umundo/connection/Publisher.h"

#include "umundo/common/Factory.h"
#include "umundo/common/Message.h"
#include "umundo/common/UUID.h"

namespace umundo {

int PublisherImpl::instances = 0;

PublisherImpl::PublisherImpl() : _greeter(NULL) {
	instances++;
}

PublisherImpl::~PublisherImpl() {
	instances--;
}

Publisher::Publisher(const std::string& channelName) {
	PublisherConfig config;
	init(ZEROMQ, channelName, NULL, &config);
}

Publisher::Publisher(const std::string& channelName, Greeter* greeter) {
	PublisherConfig config;
	init(ZEROMQ, channelName, greeter, &config);
}

Publisher::Publisher(const std::string& channelName, PublisherConfig *config) {
	init(ZEROMQ, channelName, NULL, config);
}

Publisher::Publisher(const std::string& channelName, Greeter* greeter, PublisherConfig *config) {
	init(ZEROMQ, channelName, greeter, config);
}

Publisher::Publisher(PublisherType type, const std::string& channelName) {
	PublisherConfig config;
	init(type, channelName, NULL, &config);
}

Publisher::Publisher(PublisherType type, const std::string& channelName, Greeter* greeter) {
	PublisherConfig config;
	init(type, channelName, greeter, &config);
}

Publisher::Publisher(PublisherType type, const std::string& channelName, PublisherConfig *config) {
	init(type, channelName, NULL, config);
}

Publisher::Publisher(PublisherType type, const std::string& channelName, Greeter* greeter, PublisherConfig *config) {
	init(type, channelName, greeter, config);
}


void Publisher::init(PublisherType type, const std::string& channelName, Greeter* greeter, PublisherConfig *config) {
	switch (type) {
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

	PublisherStub::_impl = _impl;
	_impl->setChannelName(channelName);
	config->channelName = channelName;
	if (greeter != NULL)
		_impl->setGreeter(greeter);

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