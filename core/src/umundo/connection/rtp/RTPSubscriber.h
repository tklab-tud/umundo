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

#include <boost/shared_ptr.hpp>
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtpaddress.h>

#include "umundo/config.h"
#include "umundo/common/Common.h"
#include "umundo/common/Message.h"
#include "umundo/connection/Subscriber.h"
#include "umundo/connection/rtp/RTPHelpers.h"

namespace umundo {

class PublisherStub;
class NodeStub;

class DLLEXPORT RTPSubscriber : public SubscriberImpl, public Thread, protected RTPHelpers {
public:
	boost::shared_ptr<Implementation> create();
	void init(Options*);
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
	void OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress);

private:
	jrtplib::RTPSession _sess;
	bool _isIPv6;
	uint8_t _payloadType;
	
	bool _isSuspended;
	
	std::multimap<std::string, std::string> _domainPubs;
	Mutex _mutex;
	
	friend class Factory;

};

}


#endif /* end of include guard: RTPSUBSCRIBER_H_HQPJWLQR */
