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

#include "PBSerializer.h"
#include "umundo/common/Factory.h"

namespace umundo {

PBSerializer::PBSerializer() {}

PBSerializer::~PBSerializer() {}

shared_ptr<Implementation> PBSerializer::create(void*) {
	shared_ptr<Implementation> instance(new PBSerializer());
	return instance;
}

void PBSerializer::destroy() {

}

void PBSerializer::init(shared_ptr<Configuration> config) {
}

string PBSerializer::serialize(const string& type, void* obj) {
	MessageLite* pbObj = (MessageLite*)obj;
	return pbObj->SerializeAsString();
}

// string PBSerializer::serialize(void* obj) {
//   return serialize("", obj);
// }

void PBSerializer::registerType(const string& type, void* serializer) {
	_serializers[type] = (MessageLite*)serializer;
}

}