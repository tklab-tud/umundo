/**
 *  @file
 *  @brief      Abstraction for Subscribers with typed Objects
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

#ifndef TYPEDSUBSCRIBER_H_ASH7AO4U
#define TYPEDSUBSCRIBER_H_ASH7AO4U

#include "umundo/common/Common.h"
#include "umundo/connection/Subscriber.h"
#include "umundo/common/Implementation.h"

namespace umundo {

class DLLEXPORT TypedReceiver {
public:
	virtual void receive(void* object, Message* msg) = 0;
};

/**
 * Base class for Type Deserializer to map strings to objects.
 */
class DLLEXPORT TypeDeserializerImpl : public Implementation {
public:
	virtual void* deserialize(const string& type, const string& data) = 0;
	virtual void destroyObj(void* obj) = 0;
	virtual void registerType(const string& type, void* deserializer) = 0;
};

/**
 * Facade for an object receiving subscriber.
 */
class DLLEXPORT TypedSubscriber : public Subscriber, public Receiver {
public:
	TypedSubscriber(string channelName);
	TypedSubscriber(string channelName, TypedReceiver* recv);
	virtual ~TypedSubscriber();
	void registerType(const string& type, void* serializer);
	void receive(Message* msg);

	virtual string getType(Message* msg);
	virtual void* deserialize(Message* msg);

private:
	shared_ptr<TypeDeserializerImpl> _impl;
	TypedReceiver* _recv;
	static TypeDeserializerImpl* _registeredPrototype; ///< The instance we registered at the factory
};

}

#endif /* end of include guard: TYPEDSUBSCRIBER_H_ASH7AO4U */
