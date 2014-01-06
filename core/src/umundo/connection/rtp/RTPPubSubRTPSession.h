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

#ifndef RTPPUBSUBRTPSESSION_H_XQPJWLQS
#define RTPPUBSUBRTPSESSION_H_XQPJWLQS

#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtpipv6address.h>
#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtptimeutilities.h>
#include <jrtplib3/rtppacket.h>
#include <jrtplib3/rtcpcompoundpacket.h>

namespace umundo {

class RTPSubscriber;

class RTPSubscriberRTPSession : public jrtplib::RTPSession {
public:
	void setSubscriber(RTPSubscriber *sub);
	void OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress);
	//void OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress);
private:
	RTPSubscriber *_sub;
};

}

#endif /* end of include guard: RTPPUBSUBRTPSESSION_H_XQPJWLQS */
