/**
 *  @file
 *  @brief      Classes to provide and consume Services
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

#ifndef SERVICE_H_8J9Z1YLY
#define SERVICE_H_8J9Z1YLY

#include <umundo/core.h>
#include <umundo/s11n.h>

namespace umundo {

class ServiceManager;
class ServiceStub;

/**
 * A ServiceDescription as returned by the ServiceManager to be used to instantiate local ServiceStub%s.
 *
 * @sa ServiceManager
 */
class DLLEXPORT ServiceDescription {
public:
	ServiceDescription(std::map<std::string, std::string>);
	ServiceDescription();

	operator bool() const {
		return _channelName.length() > 0;
	}
	bool operator< (const ServiceDescription& other) const {
		return _channelName < other._channelName;
	}
	bool operator==(const ServiceDescription& other) const {
		return _channelName == other._channelName;
	}
	bool operator!=(const ServiceDescription& other) const {
		return _channelName != other._channelName;
	}

	const std::string getName() const                      {
		return _svcName;
	}
	const std::string getChannelName() const               {
		return _channelName;
	}
	const std::map<std::string, std::string> getProperties() const  {
		return _properties;
	}
	const bool hasProperty(const std::string& key) const   {
		return _properties.find(key) != _properties.end();
	}
	const std::string getProperty(const std::string& key) const {
		std::map<std::string, std::string>::const_iterator iter = _properties.find(key);
		if (iter != _properties.end())
			return iter->second;
		return "";
	}
	void setProperty(const std::string& key, const std::string& value)   {
		_properties[key] = value;
	}

	///< We need the ServiceManagers nodes in several Services
	ServiceManager* getServiceManager() const {
		return _svcManager;
	}

protected:
	ServiceDescription(Message*);

	Message* toMessage() const;

	std::string _svcName;
	std::string _channelName;
	std::map<std::string, std::string> _properties;
	ServiceManager* _svcManager;

	friend class ServiceManager;
	friend class ServiceStub;
};

/**
 * A ServiceFilter can be used with the ServiceManager to get ServiceDescriptions.
 */
class DLLEXPORT ServiceFilter {
public:
	struct Rule {
		std::string key;
		std::string pattern;
		std::string value;
		int predicate;
	};

	ServiceFilter() {};

	operator bool() const {
		return _svcName.length() > 0;
	}
	bool operator< (const ServiceFilter& other) const {
		return _uuid < other._uuid;
	}
	bool operator==(const ServiceFilter& other) const {
		return _uuid == other._uuid;
	}
	bool operator!=(const ServiceFilter& other) const {
		return _uuid != other._uuid;
	}

	enum Predicate {
	    OP_EQUALS       = 0x0001,
	    OP_GREATER      = 0x0002,
	    OP_LESS         = 0x0003,
	    OP_STARTS_WITH  = 0x0004,
	    OP_ENDS_WITH    = 0x0005,
	    OP_CONTAINS     = 0x0006,
	    MOD_NOT         = 0x1000,
	    MASK_OP         = 0x0fff,
	    MASK_MOD        = 0xf000,
	};

	ServiceFilter(const std::string&);
	ServiceFilter(Message* msg);

	Message* toMessage() const;

	void addRule(const std::string& key, const std::string& value, int pred = OP_EQUALS);
	void addRule(const std::string& key, const std::string& pattern, const std::string& value, int pred = OP_EQUALS);
	void clearRules();
	bool matches(const ServiceDescription&) const;

	const std::string getServiceName() const {
		return _svcName;
	}
	const std::string getUUID() const {
		return _uuid;
	}

	std::vector<Rule> _rules;

private:
	std::string _uuid;
	std::string _svcName;

	bool isNumeric(const std::string& test) const;
	double toNumber(const std::string& numberString) const;

	friend class std::map<std::string, ServiceFilter>;
	friend class ServiceManager;
};

/**
 * Local representation of a remote umundo Service%s.
 */
class DLLEXPORT ServiceStub : public TypedReceiver, public Connectable {
public:
	ServiceStub(const std::string& channel);
	ServiceStub(const ServiceDescription& svcDesc);
	virtual ~ServiceStub();
	virtual const std::string getName();
	virtual const std::string getChannelName();

	// Connectable interface
	virtual std::map<std::string, Publisher> getPublishers();
	virtual std::map<std::string, Subscriber> getSubscribers();

	virtual void receive(void* object, Message* msg);

	void callStubMethod(const std::string&, void*, const std::string&, void*&, const std::string&);

protected:
	ServiceStub() {};


	std::string _channelName;
	std::string _serviceName;
	TypedPublisher _rpcPub;
	TypedSubscriber _rpcSub;

	std::map<std::string, Monitor*> _requests;
	std::map<std::string, void*> _responses;

	Mutex _mutex;

	friend class ServiceManager;
};

/**
 * Abstract base class for all Service Implementations.
 */
class DLLEXPORT Service : public ServiceStub {
public:
	Service();
	virtual ~Service();

	virtual void receive(void* object, Message* msg);

protected:
	virtual void callMethod(std::string& methodName, void* in, std::string& inType, void*& out, std::string& outType) = 0;
	/**< Generated classes overwrite this function for demarshalling.
		@param methodName The method someone is calling on a connected ServiceStub.
		@param in The argument, only one argument is allowed (Just use a compound object).
		@param inType The name of the type of the in argument from umundo.s11n - will be set if empty.
		@param out This is where we are expecting the reply.
		@param outType The name of the return type from umundo.s11n - will be set if empty.
		@sa ServiceGeneratorCPP
	*/

	virtual void cleanUpObjects(std::string& methodName, void* in, void* out) = 0;
	/**< Deallocate argument and return value after we sent our rpc reply.
		@param methodName The method someone is calling on a connected ServiceStub.
		@param in The argument, only one argument is allowed (Just use a compound object).
		@param out This is where we are expecting the reply.
		@sa ServiceGeneratorCPP
	*/

};

}


#endif /* end of include guard: SERVICE_H_8J9Z1YLY */
