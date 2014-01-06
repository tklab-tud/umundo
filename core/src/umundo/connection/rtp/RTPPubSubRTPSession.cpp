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

#include "umundo/connection/rtp/RTPPubSubRTPSession.h"
#include "umundo/connection/rtp/RTPSubscriber.h"

namespace umundo {

void RTPSubscriberRTPSession::setSubscriber(RTPSubscriber *sub) {
	_sub=sub;
}

void RTPSubscriberRTPSession::OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress) {
	if(_sub!=NULL)
		_sub->OnRTPPacket(pack, receivetime, senderaddress);
}

/*void RTPSubscriberRTPSession::OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress) {
	if(_sub!=NULL)
		_sub->OnRTCPCompoundPacket(pack, receivetime, senderaddress);
}*/

}