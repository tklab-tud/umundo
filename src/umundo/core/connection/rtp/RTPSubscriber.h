/**
 *  @file
 *  @author     2013 Thilo Molitor (thilo@eightysoft.de)
 *  @author     2013 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#ifndef RTPSUBSCRIBER_H_HQPJWLQR
#define RTPSUBSCRIBER_H_HQPJWLQR

#include "umundo/core/Common.h"

#include <queue>
#include <boost/shared_ptr.hpp>

#include "umundo/config.h"
#include "umundo/core/thread/Thread.h"
#include "umundo/core/Message.h"
#include "umundo/core/connection/Subscriber.h"
#include "umundo/core/connection/rtp/RTPThread.h"

#include "umundo/core/connection/rtp/libre.h"

namespace umundo {

class PublisherStub;
class NodeStub;

class UMUNDO_API RTPSubscriber : public SubscriberImpl, public Thread {
public:
	SharedPtr<Implementation> create();
	void init(const Options*);
	virtual ~RTPSubscriber();
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
	RTPSubscriber();

private:
	uint16_t _extendedSequenceNumber;
	uint16_t _lastSequenceNumber;

	bool _isSuspended;
	bool _initDone;

	std::queue<Message*> _queue;
	std::multimap<std::string, std::string> _domainPubs;
	RMutex _mutex;
	Monitor _cond;

	SharedPtr<RTPThread> _rtpThread;
	struct libre::rtp_sock *_rtp_socket;
	static void rtp_recv(const struct libre::sa*, const struct libre::rtp_header*, struct libre::mbuf*, void*);

	friend class Factory;
};

}


#endif /* end of include guard: RTPSUBSCRIBER_H_HQPJWLQR */
