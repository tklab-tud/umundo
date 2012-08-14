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

#include <boost/enable_shared_from_this.hpp>

#include "umundo/common/Common.h"
#include "umundo/common/ResultSet.h"
#include "umundo/connection/Subscriber.h"

namespace umundo {

class PublisherStub;

/**
 * Concrete subscriber implementor for 0MQ (bridge pattern).
 */
class DLLEXPORT ZeroMQSubscriber : public SubscriberImpl, public ResultSet<PublisherStub>, public boost::enable_shared_from_this<ZeroMQSubscriber> {
public:
	shared_ptr<Implementation> create(void*);
	void destroy();
	void init(shared_ptr<Configuration>);
	virtual ~ZeroMQSubscriber();
	void suspend();
	void resume();

	virtual Message* getNextMsg();
	virtual Message* peekNextMsg();

	void added(shared_ptr<PublisherStub>);
	void removed(shared_ptr<PublisherStub>);
	void changed(shared_ptr<PublisherStub>);

	// Thread
	void run();
	void join();

protected:
	ZeroMQSubscriber();
	ZeroMQSubscriber(string, Receiver*);

	std::list<Message*> _msgQueue;
	Mutex _msgMutex;

	set<string> _connections;
	void* _closer; ///< needed to join the thread with blocking receive
	void* _socket;
	void* _zeroMQCtx;
	Mutex _mutex;

private:

	shared_ptr<SubscriberConfig> _config;
	friend class Factory;
};

}

#endif /* end of include guard: ZEROMQSUBSCRIBER_H_6DV3QJUH */
