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
	string buffer(_impl->serialize(type, obj));
	msg->setData(buffer);
	msg->putMeta("um.s11n.type", type);
	return msg;
}

void TypedPublisher::sendObj(const string& type, void* obj) {
	Message* msg = prepareMsg(type, obj);
	send(msg);
	delete msg;
}

// void TypedPublisher::sendObj(void* obj) {
// 	Message* msg = new Message();
// 	string buffer(_impl->serialize(obj));
// 	msg->setData(buffer);
// 	send(msg);
// }

void TypedPublisher::registerType(const string& type, void* serializer) {
	_impl->registerType(type, serializer);
}

}
