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

#ifndef TYPEDPUBLISHER_H_9RTI6TXT
#define TYPEDPUBLISHER_H_9RTI6TXT

#include "umundo/common/Common.h"
#include "umundo/common/Message.h"
#include "umundo/common/Implementation.h"
#include "umundo/connection/Publisher.h"

namespace umundo {

class DLLEXPORT TypeSerializerImpl : public Implementation {
public:
	virtual string serialize(const string& type, void* obj) = 0;
	virtual void registerType(const string& type, void* serializer) = 0;
};

class DLLEXPORT TypedPublisher : public Publisher {
public:
	TypedPublisher(string channelName);
	virtual ~TypedPublisher();

	Message* prepareMsg(const string&, void*);
	void sendObj(const string& type, void* obj);
	void registerType(const string& type, void* serializer);
private:
	shared_ptr<TypeSerializerImpl> _impl;

	static TypeSerializerImpl* _registeredPrototype; ///< The instance we registered at the factory
};

}

#endif /* end of include guard: TYPEDPUBLISHER_H_9RTI6TXT */
