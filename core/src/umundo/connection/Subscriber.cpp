/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
 */

#include "umundo/connection/Subscriber.h"
#include "umundo/common/Factory.h"

namespace umundo {

shared_ptr<Configuration> SubscriberConfig::create() {
	return shared_ptr<SubscriberConfig>(new SubscriberConfig());
}

Subscriber::Subscriber(string channelName, Receiver* receiver) {
	_impl = boost::static_pointer_cast<SubscriberImpl>(Factory::create("subscriber", this));
	_config = boost::static_pointer_cast<SubscriberConfig>(Factory::config("subscriber"));
//	_config->channelName = channelName;
//	_config->receiver = receiver;
	_impl->setChannelName(channelName);
	_impl->setReceiver(receiver);
	_impl->init(_config);
}

Subscriber::~Subscriber() {
}

Subscriber::Subscriber(string channelName) {
	_impl = boost::static_pointer_cast<SubscriberImpl>(Factory::create("subscriber", this));
//	_config->channelName = channelName;
//	_config->receiver = receiver;
	_impl->setChannelName(channelName);
}

void Subscriber::setReceiver(Receiver* receiver) {
	_impl->setReceiver(receiver);
	_config = boost::static_pointer_cast<SubscriberConfig>(Factory::config("subscriber"));
	_impl->init(_config);
}

}