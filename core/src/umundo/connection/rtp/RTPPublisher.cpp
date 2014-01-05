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

#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif // WIN32

#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpudpv6transmitter.h>
#include <jrtplib3/rtpbyteaddress.h>
#include <jrtplib3/rtpsessionparams.h>
#include <jrtplib3/rtperrors.h>
#include <jrtplib3/rtptimeutilities.h>

#include "umundo/connection/rtp/RTPPublisher.h"

namespace umundo {

RTPPublisher::RTPPublisher() {
#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
#endif // WIN32
	_isSuspended=false;
	//TODO: distinguish between ipv4 and ipv6
	_isIPv6=false;
}

void RTPPublisher::init(Options* config) {
	ScopeLock lock(_mutex);
	int status;
	jrtplib::RTPSessionParams sessparams;
	jrtplib::RTPTransmissionParams *transparams;
	uint16_t portbase=strTo<uint16_t>(config->getKVPs()["pub.rtp.portbase"]);
	double samplesPerSec=strTo<double>(config->getKVPs()["pub.rtp.samplesPerSec"]);
	uint32_t timestampIncrement=strTo<uint32_t>(config->getKVPs()["pub.rtp.timestampIncrement"]);
	if(!config->getKVPs().count("pub.rtp.portbase"))
		portbase=16384;
	if(portbase==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid portbase (0 < portbase < 65536)", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(!config->getKVPs().count("pub.rtp.samplesPerSec") || samplesPerSec==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid samplesPerSec (samplesPerSec > 0)", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(!config->getKVPs().count("pub.rtp.timestampIncrement") || timestampIncrement==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid timestampIncrement (timestampIncrement > 0)", SHORT_UUID(_uuid).c_str());
		return;
	}

	_payloadType=96;		//dynamic [RFC3551]
	if(config->getKVPs().count("pub.rtp.payloadType"))
		_payloadType=strTo<uint8_t>(config->getKVPs()["pub.rtp.payloadType"]);

	// IMPORTANT: The local timestamp unit MUST be set, otherwise
	//            RTCP Sender Report info will be calculated wrong
	// We'll be sending samplesPerSec samples each second, so we'll
	// put the timestamp unit to (1.0/samplesPerSec)
	sessparams.SetOwnTimestampUnit(1.0/samplesPerSec);

	sessparams.SetAcceptOwnPackets(false);
	sessparams.SetUsePollThread(false);
	sessparams.SetReceiveMode(jrtplib::RTPTransmitter::AcceptSome);
	sessparams.SetResolveLocalHostname(false);
	sessparams.SetCNAME("um.rtp." + _channelName);
	if(_isIPv6==true)
		transparams=new jrtplib::RTPUDPv6TransmissionParams;
	else
		transparams=new jrtplib::RTPUDPv4TransmissionParams;
	for(; portbase<16384+32678; portbase+=2) {	//try to find free ports
		if(_isIPv6)
			((jrtplib::RTPUDPv6TransmissionParams*)transparams)->SetPortbase(portbase);
		else
			((jrtplib::RTPUDPv4TransmissionParams*)transparams)->SetPortbase(portbase);
		if((status=_sess.Create(sessparams, transparams, _isIPv6 ? jrtplib::RTPTransmitter::IPv6UDPProto : jrtplib::RTPTransmitter::IPv4UDPProto))>=0)
			break;
		if(config->getKVPs().count("pub.rtp.portbase"))		//only if no portbase was explicitly specified
			break;
	}
	if(status<0)
		UM_LOG_ERR("%s: error in session.Create(): %s", SHORT_UUID(_uuid).c_str(), jrtplib::RTPGetErrorString(status).c_str());
	else
		UM_LOG_WARN("%s: session.Create() using portbase: %d", SHORT_UUID(_uuid).c_str(), portbase);
	_port=portbase;
	_sess.SetDefaultPayloadType(_payloadType);
	_sess.SetDefaultMark(false);
	_sess.SetDefaultTimestampIncrement(timestampIncrement);

	start();
	delete transparams;
}

RTPPublisher::~RTPPublisher() {
	stop();
	join();
	if(_sess.IsActive())
		_sess.BYEDestroy(jrtplib::RTPTime(2,0),0,0);
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

boost::shared_ptr<Implementation> RTPPublisher::create() {
	return boost::shared_ptr<RTPPublisher>(new RTPPublisher());
}

void RTPPublisher::suspend() {
	//TODO: do something useful on android/ios (e.g. send BYE to subscribers etc.)
	ScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;
};

void RTPPublisher::resume() {
	//TODO: do something useful on android/ios (e.g. send BYE to subscribers etc.)
	//or maybe use jrtplib::RTPSession::IncrementTimestampDefault() to increment the timestamp
	//according to the time elapsed between suspend and resume
	ScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;
};

int RTPPublisher::waitForSubscribers(int count, int timeoutMs) {
	ScopeLock lock(_mutex);
	uint64_t now = Thread::getTimeStampMs();
	while (unique_keys(_domainSubs) < (unsigned int)count) {
		_pubLock.wait(_mutex, timeoutMs);
		if (timeoutMs > 0 && Thread::getTimeStampMs() - timeoutMs > now)
			break;
	}
	return unique_keys(_domainSubs);
}

void RTPPublisher::added(const SubscriberStub& sub, const NodeStub& node) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t port=sub.getPort();
	std::string ip=node.getIP();

	// do we already now about this sub via this node?
	std::pair<_domainSubs_t::iterator, _domainSubs_t::iterator> subIter = _domainSubs.equal_range(sub.getUUID());
	while(subIter.first != subIter.second) {
		if (subIter.first->second.first.getUUID() == node.getUUID())
			return; // we already know about this sub from this node
		subIter.first++;
	}

	UM_LOG_WARN("%s: received a new subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _channelName.c_str());

	const jrtplib::RTPAddress *addr=strToAddress(_isIPv6, ip, port);
	if((status=_sess.AddDestination(*addr))<0)
		UM_LOG_WARN("%s: error in session.AddDestination(%s:%u): %d - %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, status, jrtplib::RTPGetErrorString(status).c_str());
	else if((status=_sess.AddToAcceptList(*addr))<0)
		UM_LOG_WARN("%s: error in session.AddToAcceptList(%s:%u): %d - %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, status, jrtplib::RTPGetErrorString(status).c_str());
	delete addr;

	_subs[sub.getUUID()] = sub;
	_domainSubs.insert(std::make_pair(sub.getUUID(), std::make_pair(node, sub)));
	UMUNDO_SIGNAL(_pubLock);
}

void RTPPublisher::removed(const SubscriberStub& sub, const NodeStub& node) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t port=sub.getPort();
	std::string ip=node.getIP();

	// do we now about this sub via this node?
	bool subscriptionFound = false;
	std::pair<_domainSubs_t::iterator, _domainSubs_t::iterator> subIter = _domainSubs.equal_range(sub.getUUID());
	while(subIter.first != subIter.second) {
		if (subIter.first->second.first.getUUID() == node.getUUID()) {
			subscriptionFound = true;
			break;
		}
		subIter.first++;
	}
	if (!subscriptionFound)
		return;

	UM_LOG_WARN("%s: lost a subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _channelName.c_str());

	const jrtplib::RTPAddress *addr=strToAddress(_isIPv6, ip, port);
	if((status=_sess.DeleteDestination(*addr))<0)
		UM_LOG_WARN("%s: error in session.DeleteDestination(%s:%u): %d - %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, status, jrtplib::RTPGetErrorString(status).c_str());
	else if((status=_sess.DeleteFromAcceptList(*addr))<0)
		UM_LOG_WARN("%s: error in session.DeleteFromAcceptList(%s:%u): %d - %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, status, jrtplib::RTPGetErrorString(status).c_str());
	delete addr;

	if (_domainSubs.count(sub.getUUID()) == 1) { // about to vanish
		_subs.erase(sub.getUUID());
	}

	_domainSubs.erase(subIter.first);
	UMUNDO_SIGNAL(_pubLock);
}

void RTPPublisher::send(Message* msg) {
	int status;
	std::string timestampIncrement=msg->getMeta("timestampIncrement");
	if(!timestampIncrement.size())
		timestampIncrement=_mandatoryMeta["timestampIncrement"];
	if(timestampIncrement.size())
		status=_sess.SendPacket(msg->data(), msg->size(), _payloadType, false, strTo<uint32_t>(timestampIncrement));
	else
		status=_sess.SendPacket(msg->data(), msg->size());
	if(status<0)
		UM_LOG_WARN("%s: error in session.SendPacket(): %s", SHORT_UUID(_uuid).c_str(), jrtplib::RTPGetErrorString(status).c_str());
}

void RTPPublisher::run() {
	int status;
	while(isStarted()) {
		{
			ScopeLock lock(_mutex);
			if((status=_sess.Poll())<0)
				UM_LOG_WARN("%s: error in session.Poll(): %s", SHORT_UUID(_uuid).c_str(), jrtplib::RTPGetErrorString(status).c_str());
		}
		jrtplib::RTPTime::Wait(jrtplib::RTPTime(1,0));
	}
}

}
