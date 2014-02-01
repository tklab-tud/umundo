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

#include "umundo/s11n/TypedSubscriber.h"
#include "umundo/common/Factory.h"
#include "umundo/common/Message.h"

#include "umundo/config.h"
#ifdef S11N_PROTOBUF
#include "umundo/s11n/protobuf/PBDeserializer.h"
#else
#error No serialization implementation choosen
#endif

namespace umundo {

TypeDeserializerImpl::~TypeDeserializerImpl() {
}

void TypeDeserializerImpl::receive(Message* msg) {
	if (msg->getMeta().find("um.s11n.type") != msg->getMeta().end()) {
		// explicit type given and known
		void* obj = deserialize(msg->getMeta("um.s11n.type"), msg);
		_recv->receive(obj, msg);
		//    _impl->destroyObj(obj);
	} else {
		// generic message
		void* obj = deserialize("", msg);
		_recv->receive(obj, msg);
	}
}

TypedSubscriber::TypedSubscriber(const std::string& channelName) : Subscriber(channelName) {
	if (_registeredPrototype == NULL) {
#ifdef S11N_PROTOBUF
		_registeredPrototype = new PBDeserializer();
#endif
		Factory::registerPrototype("typeDeserializer", _registeredPrototype);
	}
	_impl = StaticPtrCast<TypeDeserializerImpl>(Factory::create("typeDeserializer"));
	assert(_impl != NULL);
}

TypedSubscriber::TypedSubscriber(const std::string& channelName, TypedReceiver* recv) : Subscriber(channelName) {
	if (_registeredPrototype == NULL) {
#ifdef S11N_PROTOBUF
		_registeredPrototype = new PBDeserializer();
#endif
		Factory::registerPrototype("typeDeserializer", _registeredPrototype);
	}
	_impl = StaticPtrCast<TypeDeserializerImpl>(Factory::create("typeDeserializer"));
	_impl->setReceiver(recv);
	Subscriber::setReceiver(_impl.get());
	assert(_impl != NULL);
}
TypeDeserializerImpl* TypedSubscriber::_registeredPrototype = NULL;

TypedSubscriber::~TypedSubscriber() {
	// TODO: Copying will unset received, but destroying will segfault otherwise - we need ref counting
	if (Subscriber::_impl.use_count() <= 2) { // oh my, what a hack
		Subscriber::setReceiver(NULL);
	}
}

void TypedSubscriber::registerType(const std::string& type, void* deserializer) {
	_impl->registerType(type, deserializer);
}

std::string TypedSubscriber::getType(Message* msg) {
	if (msg->getMeta().find("um.s11n.type") != msg->getMeta().end()) {
		return msg->getMeta("um.s11n.type");
	}
	return "";
}

void* TypedSubscriber::deserialize(Message* msg) {
	if (msg->getMeta().find("um.s11n.type") != msg->getMeta().end()) {
		return _impl->deserialize(msg->getMeta("um.s11n.type"), msg);
	}
	return NULL;
}

}
