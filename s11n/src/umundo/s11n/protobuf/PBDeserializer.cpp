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

#include "umundo/s11n/protobuf/PBDeserializer.h"

namespace umundo {

void PBDeserializer::destroy() {}

void PBDeserializer::init(shared_ptr<Configuration>) {}

shared_ptr<Implementation> PBDeserializer::create(void*) {
	shared_ptr<Implementation> instance(new PBDeserializer());
	return instance;
}

// void* PBDeserializer::deserialize(const string& data) {
// 	MessageLite* pbObj = new PBTypedMessage();
// 	pbObj->ParseFromString(data);
// 	// \todo: unpack eventual custom type
// 	return pbObj;
// }

void* PBDeserializer::deserialize(const string& type, const string& data) {
	if (_deserializers.find(type) == _deserializers.end()) {
		LOG_ERR("received type %s, but no deserializer is known", type.c_str());
		return NULL;
	}
	MessageLite* pbObj = _deserializers[type]->New();
	pbObj->ParseFromString(data);
	return pbObj;
}

void PBDeserializer::destroyObj(void* obj) {
	delete (MessageLite*)obj;
}

void PBDeserializer::registerType(const std::string& type, void* deserializer) {
	_deserializers[type] = (MessageLite*)deserializer;
}

}