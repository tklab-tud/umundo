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

#include "umundo/s11n/protobuf/PBDeserializer.h"
#include "umundo/s11n/protobuf/PBSerializer.h"
#include <google/protobuf/dynamic_message.h>

namespace umundo {

void PBDeserializer::destroy() {}

void PBDeserializer::init(shared_ptr<Configuration>) {}

shared_ptr<Implementation> PBDeserializer::create(void*) {
	shared_ptr<Implementation> instance(new PBDeserializer());
	return instance;
}

void* PBDeserializer::deserialize(const string& type, Message* msg) {
  const string data(msg->data(), msg->size());
	if (_deserializers.find(type) != _deserializers.end()) {
    // we have an explicit deserializer type registered
    MessageLite* pbObj = _deserializers[type]->New();
    if(!pbObj->ParseFromString(data)) {
      LOG_ERR("Could not parse %s from serialized data", type.c_str());
    }
    return pbObj;
	} else {
    // we only now the type by its description -> receive will have to use the protobuf message API!
    const google::protobuf::Message* deserializer = PBSerializer::getProto(type);
    if (deserializer != NULL) {
      assert(deserializer->GetTypeName().compare(type) == 0);
      google::protobuf::Message* pbObj = deserializer->New();
      if(!pbObj->ParseFromString(data)) {
        LOG_ERR("Could not parse %s from serialized data", type.c_str());
      }
      // make sure we do not try to cast it to the actual object in receive
      msg->putMeta("um.s11n.type", "google::protobuf::Message");
      return pbObj;
    }
  }
  LOG_ERR("received type %s, but no deserializer is known", type.c_str());
  return NULL;
}

void PBDeserializer::destroyObj(void* obj) {
	delete (MessageLite*)obj;
}

void PBDeserializer::registerType(const std::string& type, void* deserializer) {
	_deserializers[type] = (MessageLite*)deserializer;
}

}