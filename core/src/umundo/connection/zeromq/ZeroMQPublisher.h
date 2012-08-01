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

#ifndef ZEROMQPUBLISHER_H_AX5HLY5Q
#define ZEROMQPUBLISHER_H_AX5HLY5Q

#include <boost/enable_shared_from_this.hpp>

#include "umundo/common/Common.h"
#include "umundo/connection/Publisher.h"
#include "umundo/thread/Thread.h"

namespace umundo {

class ZeroMQNode;

/**
 * Concrete publisher implementor for 0MQ (bridge pattern).
 */
class DLLEXPORT ZeroMQPublisher : public PublisherImpl, public Thread, public boost::enable_shared_from_this<ZeroMQPublisher>  {
public:
	virtual ~ZeroMQPublisher();

	shared_ptr<Implementation> create(void*);
	void init(shared_ptr<Configuration>);
	void destroy();
	void suspend();
	void resume();

	void send(Message* msg);
	int waitForSubscribers(int count, int timeoutMs);

protected:
	/**
	* Constructor used for prototype in Factory only.
	*/
	ZeroMQPublisher();
	void addedSubscriber(const string, const string);
	void removedSubscriber(const string, const string);

private:
	// ZeroMQPublisher(const ZeroMQPublisher &other) {}
	// ZeroMQPublisher& operator= (const ZeroMQPublisher &other) {
	// 	return *this;
	// }

	// read subscription requests from publisher socket
	void run();
	void join();

	void* _socket;
	void* _closer;
	void* _zeroMQCtx;
	shared_ptr<PublisherConfig> _config;

	/**
	 * To ensure solid subscriptions, we receive them twice,
	 * once through the node socket and once through the
	 * xpub socket, only when both have been received do we
	 * we signal the greeters.
	 */
	set<string> _pendingZMQSubscriptions;
	map<string, string> _pendingSubscriptions;
	map<string, string> _subscriptions;

	Monitor _pubLock;
	Mutex _mutex;

	friend class Factory;
	friend class ZeroMQNode;
	friend class Publisher;
};

}

#endif /* end of include guard: ZEROMQPUBLISHER_H_AX5HLY5Q */
