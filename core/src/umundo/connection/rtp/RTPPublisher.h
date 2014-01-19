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

#ifndef RTPPUBLISHER_H_H9LXV94P
#define RTPPUBLISHER_H_H9LXV94P

#include <boost/shared_ptr.hpp>
#include <jrtplib3/rtpsession.h>

#include "umundo/common/Common.h"
#include "umundo/common/Message.h"
#include "umundo/connection/Publisher.h"
#include "umundo/thread/Thread.h"
#include "umundo/connection/rtp/RTPHelpers.h"

namespace umundo {

/**
 * Concrete publisher implementor for RTP (bridge pattern).
 */
class DLLEXPORT RTPPublisher : public PublisherImpl, public Thread, protected RTPHelpers {
public:
	virtual ~RTPPublisher();

	boost::shared_ptr<Implementation> create();
	void init(Options*);
	void suspend();
	void resume();

	void send(Message* msg);
	int waitForSubscribers(int count, int timeoutMs);

protected:
	/**
	 * Constructor used for prototype in Factory only.
	 */
	RTPPublisher();

	void added(const SubscriberStub& sub, const NodeStub& node);
	void removed(const SubscriberStub& sub, const NodeStub& node);

private:
	void run();
	jrtplib::RTPSession _sess;
	bool _isIPv6;
	uint8_t _payloadType;
	uint32_t _timestampIncrement;
	std::string _multicastIP;
	uint16_t _multicastPortbase;

	std::multimap<std::string, std::pair<NodeStub, SubscriberStub> > _domainSubs;
	typedef std::multimap<std::string, std::pair<NodeStub, SubscriberStub> > _domainSubs_t;

	Monitor _pubLock;
	Mutex _mutex;
	bool _isSuspended;
	bool _multicast;

	friend class Factory;

};

}


#endif /* end of include guard: RTPPUBLISHER_H_H9LXV94P */
