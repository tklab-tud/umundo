/**
 *  @file
 *  @brief      Concrete TypeSerializer with ProtoBuf
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

#ifndef PBSERIALIZER_H_LQKL8UQG
#define PBSERIALIZER_H_LQKL8UQG

#include "umundo/s11n/TypedPublisher.h"
#include <google/protobuf/message_lite.h>

using google::protobuf::MessageLite;

namespace umundo {

/**
 * TypeSerializerImpl implementor with ProtoBuf.
 */
class DLLEXPORT PBSerializer : public TypeSerializerImpl {
public:
	PBSerializer();
	virtual ~PBSerializer();

	virtual shared_ptr<Implementation> create(void*);
	virtual void destroy();
	virtual void init(shared_ptr<Configuration>);

	virtual string serialize(const string& type, void* obj);
//		virtual string serialize(void* obj);
	virtual void registerType(const string& type, void* serializer);

private:
	map<string, MessageLite*> _serializers;
};

}

#endif /* end of include guard: PBSERIALIZER_H_LQKL8UQG */
