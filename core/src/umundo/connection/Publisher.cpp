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
	_impl = boost::static_pointer_cast<PublisherImpl>(Factory::create("publisher"));
	PublisherStub::_impl = _impl;
	EndPoint::_impl = _impl;
	_impl->setChannelName(channelName);
	PublisherConfig config;
	config.channelName = channelName;
	_impl->init(&config);
}

Publisher::Publisher(const std::string& channelName, Greeter* greeter) {
	_impl = boost::static_pointer_cast<PublisherImpl>(Factory::create("publisher"));
	PublisherStub::_impl = _impl;
	_impl->setChannelName(channelName);
	PublisherConfig* config = new PublisherConfig();
	config->channelName = channelName;
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