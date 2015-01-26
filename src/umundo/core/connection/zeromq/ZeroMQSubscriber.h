/**
 *  @file
 *  @brief      Subscriber implementation with 0MQ.
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


#ifndef ZEROMQSUBSCRIBER_H_6DV3QJUH
#define ZEROMQSUBSCRIBER_H_6DV3QJUH

#include "umundo/core/Common.h"
#include "umundo/core/ResultSet.h"
#include "umundo/core/connection/Subscriber.h"

namespace umundo {

class PublisherStub;
class NodeStub;

/**
 * Concrete subscriber implementor for 0MQ (bridge pattern).
 */
class UMUNDO_API ZeroMQSubscriber : public SubscriberImpl, public Thread {
public:
	SharedPtr<Implementation> create();
	void init(const Options*);
	virtual ~ZeroMQSubscriber();
	void suspend();
	void resume();

	void setReceiver(umundo::Receiver* receiver);
	virtual Message* getNextMsg();
	virtual bool hasNextMsg();

	void added(const PublisherStub& pub, const NodeStub& node);
	void removed(const PublisherStub& pub, const NodeStub& node);

	// Thread
	void run();

protected:
	ZeroMQSubscriber();

	void* _subSocket;
	void* _readOpSocket;
	void* _writeOpSocket;
	std::multimap<std::string, std::string> _domainPubs;
	RMutex _mutex;

private:

	friend class Factory;
};

}

#endif /* end of include guard: ZEROMQSUBSCRIBER_H_6DV3QJUH */
