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

#ifndef SUBSCRIBER_H_J64J09SP
#define SUBSCRIBER_H_J64J09SP

#include "umundo/common/Common.h"
#include "umundo/common/Implementation.h"
#include "umundo/thread/Thread.h"

namespace umundo {

class DLLEXPORT Message;
class DLLEXPORT SubscriberImpl;

/**
 * Interface for client classes to get byte-arrays from subscribers.
 */
class DLLEXPORT Receiver {
public:
	virtual ~Receiver() {}
	virtual void receive(Message* msg) = 0;
	friend class Subscriber;
};

class DLLEXPORT SubscriberConfig : public Configuration {
public:
	virtual ~SubscriberConfig() {}
	shared_ptr<Configuration> create();
	string channelName;
	string uuid;
	Receiver* receiver;
};

/**
 * Subscriber implementor basis class (bridge pattern).
 */
class DLLEXPORT SubscriberImpl : public Thread, public Implementation {
public:
	virtual const string& getChannelName()           {
		return _channelName;
	}
	virtual void setChannelName(string channelName)  {
		_channelName = channelName;
	}
	virtual const Receiver* getReceiver()            {
		return _receiver;
	}
	virtual void setReceiver(Receiver* receiver)     {
		_receiver = receiver;
	}
	virtual const string& getUUID()                  {
		return _uuid;
	}
	virtual void setUUID(string uuid)                {
		_uuid = uuid;
	}

protected:
	Receiver* _receiver;
	string _channelName;
	string _uuid;
};

/**
 * Subscriber abstraction (bridge pattern).
 *
 * We need to overwrite everything to use the concrete implementors functions. The preferred
 * constructor is the Subscriber(string channelName, Receiver* receiver) one, the unqualified
 * constructor without a receiver and the setReceiver method are required for Java as we cannot
 * inherit publishers while being its receiver at the same time as is used for the TypedSubscriber.
 */
class DLLEXPORT Subscriber {
public:
	Subscriber(string channelName);
	Subscriber(string channelName, Receiver* receiver);
	virtual ~Subscriber();

	void setReceiver(Receiver* receiver);

	virtual const string& getChannelName()           {
		return _impl->getChannelName();
	}
	virtual void setChannelName(string channelName)  {
		_impl->setChannelName(channelName);
	}

protected:

	shared_ptr<SubscriberImpl> _impl;
	shared_ptr<SubscriberConfig> _config;
	friend class Node;
};

}


#endif /* end of include guard: SUBSCRIBER_H_J64J09SP */
