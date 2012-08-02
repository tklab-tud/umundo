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
	ServiceDescription(const string&, map<string, string>);
	ServiceDescription(const string&);

	const string getName()                           { return _svcName; }
	const string getChannelName()                    { return _channelName; }
	const map<string, string>& getProperties()       { return _properties; }
	const string getProperty(const string& key)      { return _properties[key]; }
	void setProperty(const string& key, const string& value)   { _properties[key] = value; }

	///< We need the ServiceManagers nodes in several Services
	ServiceManager* getServiceManager() {
		return _svcManager;
	}

protected:
	ServiceDescription(Message*);

	Message* toMessage();

	string _svcName;
	string _channelName;
	std::map<string, string> _properties;
	ServiceManager* _svcManager;

	friend class ServiceManager;
	friend class ServiceStub;
};

/**
 * A ServiceFilter can be used with the ServiceManager to get ServiceDescriptions.
 */
class DLLEXPORT ServiceFilter {
public:
  struct filterCmp {
    bool operator()(const ServiceFilter* a, const ServiceFilter* b) {
      return a->_uuid.compare(b->_uuid) < 0;
    }
  };

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

	ServiceFilter(const string&);
	ServiceFilter(Message* msg);

	Message* toMessage();

	void addRule(const string& key, const string& value, int pred = OP_EQUALS);
	void addRule(const string& key, const string& pattern, const string& value, int pred = OP_EQUALS);
	void clearRules();
	bool matches(ServiceDescription*);

  const string& getServiceName() { return _svcName; }
  const string& getUUID() { return _uuid; }

	map<string, string> _pattern;
	map<string, string> _value;
	map<string, int> _predicate;

private:
  string _uuid;
	string _svcName;

	bool isNumeric(const string& test);
	double toNumber(const string& numberString);

  friend class ServiceManager;
};

/**
 * Local representation of a remote umundo Service%s.
 */
class DLLEXPORT ServiceStub : public TypedReceiver, public Connectable {
public:
	ServiceStub(const string& channel);
	ServiceStub(ServiceDescription* svcDesc);
	virtual ~ServiceStub();
	virtual const string& getName();
	virtual const string& getChannelName();

	// Connectable interface
	virtual std::set<umundo::Publisher*> getPublishers();
	virtual std::set<umundo::Subscriber*> getSubscribers();

	virtual void receive(void* object, Message* msg);

protected:
	ServiceStub() {};

	void callStubMethod(const string&, void*, const string&, void*&, const string&);

	string _channelName;
	string _serviceName;
	TypedPublisher* _rpcPub;
	TypedSubscriber* _rpcSub;

	map<string, Monitor*> _requests;
	map<string, void*> _responses;

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
	virtual void callMethod(string& methodName, void* in, const string& inType, void*& out, const string& outType) = 0;
	/**< Generated classes overwrite this function for demarshalling.
		@param methodName The method someone is calling on a connected ServiceStub.
		@param in The argument, only one argument is allowed (Just use a compound object).
		@param inType The name of the type of the in argument from umundo.s11n.
		@param out This is where we are expecting the reply.
		@param outType The name of the return type from umundo.s11n.
		@sa ServiceGeneratorCPP
	*/

	virtual void cleanUpObjects(string& methodName, void* in, void* out) = 0;
	/**< Deallocate argument and return value after we sent our rpc reply.
		@param methodName The method someone is calling on a connected ServiceStub.
		@param in The argument, only one argument is allowed (Just use a compound object).
		@param out This is where we are expecting the reply.
		@sa ServiceGeneratorCPP
	*/

};

}


#endif /* end of include guard: SERVICE_H_8J9Z1YLY */
