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

#ifndef PBDESERIALIZER_H_DD3C36Z7
#define PBDESERIALIZER_H_DD3C36Z7

#include "umundo/s11n/TypedSubscriber.h"
#include <google/protobuf/message_lite.h>

using google::protobuf::MessageLite;

namespace umundo {
class DLLEXPORT PBDeserializer : public TypeDeserializerImpl {
public:
	PBDeserializer() {}
	virtual ~PBDeserializer() {}

	void destroy();
	void init(shared_ptr<Configuration>);
	shared_ptr<Implementation> create(void*);
	void* deserialize(const string& type, const string& data);
	void destroyObj(void* obj);

	void registerType(const std::string&, void*);

protected:
	map<string, MessageLite*> _deserializers;

};
}

#endif /* end of include guard: PBDESERIALIZER_H_DD3C36Z7 */
