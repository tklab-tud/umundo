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

#include "umundo/s11n/TypedPublisher.h"
#include "umundo/common/Factory.h"
#include "umundo/common/Message.h"

#include "umundo/config.h"
#ifdef S11N_PROTOBUF
#include "umundo/s11n/protobuf/PBSerializer.h"
#else
#error No serialization implementation choosen
#endif

namespace umundo {

TypedPublisher::TypedPublisher(string channelName) : Publisher(channelName) {
	_greeterWrapper = NULL;
	if (_registeredPrototype == NULL) {
#ifdef S11N_PROTOBUF
		_registeredPrototype = new PBSerializer();
#endif
		Factory::registerPrototype("typeSerializer", _registeredPrototype, NULL);
	}
	_impl = boost::static_pointer_cast<TypeSerializerImpl>(Factory::create("typeSerializer"));
	assert(_impl != NULL);
}
TypeSerializerImpl* TypedPublisher::_registeredPrototype = NULL;

TypedPublisher::~TypedPublisher() {
}

Message* TypedPublisher::prepareMsg(const string& type, void* obj) {
	Message* msg = new Message();
	prepareMsg(msg, type, obj);
	return msg;
}

void TypedPublisher::prepareMsg(Message* msg, const string& type, void* obj) {
	string buffer = _impl->serialize(type, obj);
	msg->setData(buffer);
	msg->putMeta("um.s11n.type", type);
}


void TypedPublisher::sendObj(const string& type, void* obj) {
	Message* msg = prepareMsg(type, obj);
	send(msg);
	delete msg;
}

void TypedPublisher::setGreeter(TypedGreeter* greeter) {
	if (_greeterWrapper == NULL) {
		_greeterWrapper = new TypedPublisher::GreeterWrapper(greeter, this);
		Publisher::setGreeter(_greeterWrapper);
	} else {
		_greeterWrapper->_typedGreeter = greeter;
	}
}

void TypedPublisher::registerType(const string& type, void* serializer) {
	_impl->registerType(type, serializer);
}

TypedPublisher::GreeterWrapper::GreeterWrapper(TypedGreeter* typedGreeter, TypedPublisher* typedPub) : _typedGreeter(typedGreeter), _typedPub(typedPub) {}

TypedPublisher::GreeterWrapper::~GreeterWrapper() {}

void TypedPublisher::GreeterWrapper::welcome(Publisher* atPub, const std::string& nodeId, const std::string& subId) {
	if (_typedGreeter != NULL) {
		_typedGreeter->welcome(_typedPub, nodeId, subId);
	}
}

void TypedPublisher::GreeterWrapper::farewell(Publisher* fromPub, const std::string& nodeId, const std::string& subId) {
	if (_typedGreeter != NULL) {
		_typedGreeter->farewell(_typedPub, nodeId, subId);
	}
}

}
