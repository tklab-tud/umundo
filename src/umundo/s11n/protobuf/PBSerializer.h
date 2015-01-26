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
#include <google/protobuf/descriptor.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>

using google::protobuf::MessageLite;
using google::protobuf::DescriptorPool;
using google::protobuf::DynamicMessageFactory;

namespace umundo {

class PBErrorReporter;

/**
 * TypeSerializerImpl implementor with ProtoBuf.
 */
class UMUNDO_API PBSerializer : public TypeSerializerImpl {
public:
	PBSerializer();
	virtual ~PBSerializer();

	virtual SharedPtr<Implementation> create();
	virtual void init(const Options*);

	virtual std::string serialize(const std::string& type, void* obj);
//		virtual string serialize(void* obj);
	virtual void registerType(const std::string& type, void* serializer);

	static void addProto(const std::string& dirOrFile);
	static const google::protobuf::Message* getProto(const std::string& type);

private:
	static void addProtoRecurse(const std::string& dirRoot, const std::string& dirOrFile, google::protobuf::compiler::Importer* importer);
	static bool isDir(const std::string& dirOrFile);

	std::map<std::string, MessageLite*> _serializers;
	static std::map<std::string, const google::protobuf::Descriptor*> descs;
	static std::map<std::string, google::protobuf::compiler::Importer*> descImporters;
	static DynamicMessageFactory* descFactory;
	static DescriptorPool* descPool;
	static PBErrorReporter* errorReporter;
	static google::protobuf::compiler::DiskSourceTree* sourceTree;

	static RMutex protoMutex;
};

class UMUNDO_API PBErrorReporter : public google::protobuf::compiler::MultiFileErrorCollector {
public:
	PBErrorReporter() {}
	virtual ~PBErrorReporter() {}
	virtual void AddError(const std::string & filename, int line, int column, const std::string & message);
};


}

#endif /* end of include guard: PBSERIALIZER_H_LQKL8UQG */
