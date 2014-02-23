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
#include "umundo/common/Message.h"

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
	TypeDeserializerImpl() {}
	virtual ~TypeDeserializerImpl();
	virtual void* deserialize(const std::string& type, Message* msg) = 0;
	virtual void destroyObj(void* obj) = 0;
	virtual void registerType(const std::string& type, void* deserializer) = 0;

protected:
};

class DLLEXPORT TypedSubscriberImpl : public Receiver, public EnableSharedFromThis<TypedSubscriberImpl> {
public:
	TypedSubscriberImpl();
	void receive(Message* msg);
	virtual void registerType(const std::string& type, void* deserializer) {
		return _impl->registerType(type, deserializer);
	}
	virtual void* deserialize(const std::string& type, Message* msg) {
		return _impl->deserialize(type, msg);
	}

	virtual void setReceiver(TypedReceiver* recv) {
		_recv = recv;
	}
	
protected:
	Subscriber _sub;
	TypedReceiver* _recv;
	SharedPtr<TypeDeserializerImpl> _impl;
};
	
/**
 * Facade for an object receiving subscriber.
 */
class DLLEXPORT TypedSubscriber : public Subscriber {
public:

	TypedSubscriber() : _impl() {}
	TypedSubscriber(const std::string& channelName);
	TypedSubscriber(const std::string& channelName, TypedReceiver* receiver);
	TypedSubscriber(SharedPtr<TypedSubscriberImpl> const impl) : _impl(impl) { }
	TypedSubscriber(const TypedSubscriber& other) : Subscriber(other), _impl(other._impl) { }
	virtual ~TypedSubscriber();

	operator bool() const {
		return _impl.get();
	}
	bool operator< (const TypedSubscriber& other) const {
		return _impl < other._impl;
	}
	bool operator==(const TypedSubscriber& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const TypedSubscriber& other) const {
		return _impl != other._impl;
	}

	TypedSubscriber& operator=(const TypedSubscriber& other) {
		Subscriber::operator=(other);
		_impl = other._impl;
		return *this;
	} // operator=

	void registerType(const std::string& type, void* deserializer) {
		_impl->registerType(type, deserializer);
	}

	virtual void setReceiver(TypedReceiver* recv) {
		if (recv != NULL) {
			Subscriber::setReceiver(_impl.get());
			_impl->setReceiver(recv);
		} else {
			Subscriber::setReceiver(NULL);
			_impl->setReceiver(recv);
		}
	}

	virtual std::string getType(Message* msg) {
		if (msg->getMeta().find("um.s11n.type") != msg->getMeta().end()) {
			return msg->getMeta("um.s11n.type");
		}
		return "";
	}
	
	virtual void* deserialize(Message* msg) {
		if (msg->getMeta().find("um.s11n.type") != msg->getMeta().end()) {
			return _impl->deserialize(msg->getMeta("um.s11n.type"), msg);
		}
		return NULL;
	}

private:
	SharedPtr<TypedSubscriberImpl> _impl;
	static TypeDeserializerImpl* _registeredPrototype; ///< The instance we registered at the factory
};

}

#endif /* end of include guard: TYPEDSUBSCRIBER_H_ASH7AO4U */
