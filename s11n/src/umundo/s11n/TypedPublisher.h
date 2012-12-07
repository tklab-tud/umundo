/**
 *  @file
 *  @brief      Abstraction for Publisher with typed Objects
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

#ifndef TYPEDPUBLISHER_H_9RTI6TXT
#define TYPEDPUBLISHER_H_9RTI6TXT

#include "umundo/common/Common.h"
#include "umundo/common/Message.h"
#include "umundo/common/Implementation.h"
#include "umundo/connection/Publisher.h"

namespace umundo {

class TypedPublisher;

/**
 * Base class for Type Serializer to map objects to strings.
 */
class DLLEXPORT TypeSerializerImpl : public Implementation {
public:
	virtual string serialize(const string& type, void* obj) = 0;
	virtual void registerType(const string& type, void* serializer) = 0;
};

/**
 * Base class for Typed Greeters.
 */
class DLLEXPORT TypedGreeter {
public:
	virtual void welcome(TypedPublisher atPub, const std::string& nodeId, const std::string& subId) = 0;
	virtual void farewell(TypedPublisher fromPub, const std::string& nodeId, const std::string& subId) = 0;
};

/**
 * Facade for an object sending publisher.
 */
class DLLEXPORT TypedPublisher : public Publisher {
private:
	/**
	 * Untyped greeter to wrap a TypedGreeter for a TypedPublisher.
	 */
	class DLLEXPORT GreeterWrapper : public Greeter {
	public:
		GreeterWrapper(TypedGreeter* typedGreeter, TypedPublisher* typedPub);
		virtual ~GreeterWrapper();
		void welcome(Publisher atPub, const std::string& nodeId, const std::string& subId);
		void farewell(Publisher fromPub, const std::string& nodeId, const std::string& subId);

	protected:
		TypedGreeter* _typedGreeter;
		TypedPublisher* _typedPub;

		friend class TypedPublisher;
	};

public:
	TypedPublisher() : _impl() {}
	TypedPublisher(const std::string& channelName);
	TypedPublisher(boost::shared_ptr<TypeSerializerImpl> const impl) :  _impl(impl) { }
	TypedPublisher(const TypedPublisher& other) : _impl(other._impl) { }
	virtual ~TypedPublisher();

	operator bool() const {
		return _impl;
	}
	bool operator< (const TypedPublisher& other) const {
		return _impl < other._impl;
	}
	bool operator==(const TypedPublisher& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const TypedPublisher& other) const {
		return _impl != other._impl;
	}


	TypedPublisher& operator=(const TypedPublisher& other) {
		Publisher::operator=(other);
		_impl = other._impl;
		return *this;
	} // operator=

	Message* prepareMsg(const string&, void* obj);
	void prepareMsg(Message* msg, const string& type, void* obj);
	void sendObj(const string& type, void* obj);
	void registerType(const string& type, void* serializer);
	void setGreeter(TypedGreeter* greeter);
private:
	shared_ptr<TypeSerializerImpl> _impl;

	GreeterWrapper* _greeterWrapper;
	static TypeSerializerImpl* _registeredPrototype; ///< The instance we registered at the factory
};

}

#endif /* end of include guard: TYPEDPUBLISHER_H_9RTI6TXT */
