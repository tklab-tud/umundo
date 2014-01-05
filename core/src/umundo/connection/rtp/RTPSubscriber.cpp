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

#include "umundo/connection/rtp/RTPSubscriber.h"

namespace umundo {

RTPSubscriber::RTPSubscriber() {
#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
#endif // WIN32
	_isSuspended=false;
	//TODO: distinguish between ipv4 and ipv6
	_isIPv6=false;

}

void RTPSubscriber::init(Options* config) {
	ScopeLock lock(_mutex);
	int status;
	jrtplib::RTPSessionParams sessparams;
	jrtplib::RTPTransmissionParams *transparams;
	uint16_t portbase=strTo<uint16_t>(config->getKVPs()["sub.rtp.portbase"]);
	if(!config->getKVPs().count("sub.rtp.portbase"))
		portbase=16384;
	if(portbase==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid portbase (0 < portbase < 65536)", SHORT_UUID(_uuid).c_str());
		return;
	}
	_payloadType=96;

	//setting this to zero suppresses RTCP packets, setting this to one should be reasonable for periodic RTCP receiver reports
	sessparams.SetOwnTimestampUnit(1);
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
		if(config->getKVPs().count("sub.rtp.portbase"))		//only if no portbase was explicitly specified
			break;
	}
	if(status<0)
		UM_LOG_ERR("%s: error in session.Create(): %s", SHORT_UUID(_uuid).c_str(), jrtplib::RTPGetErrorString(status).c_str());
	else
		UM_LOG_WARN("%s: session.Create() using portbase: %d", SHORT_UUID(_uuid).c_str(), portbase);
	_port=portbase;		//sent to publisher (via framework)
	_sess.SetDefaultPayloadType(_payloadType);
	_sess.SetDefaultMark(false);

	start();
	delete transparams;
}

RTPSubscriber::~RTPSubscriber() {
	stop();
	join();
	if(_sess.IsActive())
		_sess.BYEDestroy(jrtplib::RTPTime(2,0),0,0);
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

boost::shared_ptr<Implementation> RTPSubscriber::create() {
	return boost::shared_ptr<RTPSubscriber>(new RTPSubscriber());
}

void RTPSubscriber::suspend() {
	//TODO: do something useful on android/ios (e.g. send BYE to publishers etc.)
	ScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;
}

void RTPSubscriber::resume() {
	//TODO: do something useful on android/ios (e.g. send BYE to publishers etc.)
	//or maybe use jrtplib::RTPSession::IncrementTimestampDefault() to increment the timestamp
	//according to the time elapsed between suspend and resume
	ScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;
}

void RTPSubscriber::added(const PublisherStub& pub, const NodeStub& node) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t port=pub.getPort();
	std::string ip=node.getIP();

	if(_domainPubs.count(pub.getDomain()) == 0) {
		UM_LOG_WARN("%s: subscribing to %s (%s:%d)", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ip.c_str(), port);

		jrtplib::RTPAddress *addr=strToAddress(_isIPv6, ip, port);
		//if((status=_sess.AddDestination(*addr))<0)
		//	UM_LOG_WARN("%s: error in session.AddDestination(%s:%u): %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, jrtplib::RTPGetErrorString(status).c_str());
		if((status=_sess.AddToAcceptList(*addr))<0)
			UM_LOG_WARN("%s: error in session.AddToAcceptList(%s:%u): %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, jrtplib::RTPGetErrorString(status).c_str());
		delete addr;
	}
	_pubs[pub.getUUID()] = pub;
	_domainPubs.insert(std::make_pair(pub.getDomain(), pub.getUUID()));
}

void RTPSubscriber::removed(const PublisherStub& pub, const NodeStub& node) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t port=pub.getPort();
	std::string ip=node.getIP();

	// TODO: This fails for publishers added via different nodes
	if (_pubs.find(pub.getUUID()) != _pubs.end())
		_pubs.erase(pub.getUUID());

	if (_domainPubs.count(pub.getDomain()) == 0)
		return;

	std::multimap<std::string, std::string>::iterator domIter = _domainPubs.find(pub.getDomain());

	while(domIter != _domainPubs.end()) {
		if (domIter->second == pub.getUUID()) {
			_domainPubs.erase(domIter++);
		} else {
			domIter++;
		}
	}

	if (_domainPubs.count(pub.getDomain()) == 0) {
		UM_LOG_WARN("%s unsubscribing from %s (%s:%d)", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ip.c_str(), port);

		const jrtplib::RTPAddress *addr=strToAddress(_isIPv6, ip, port);
		//if((status=_sess.DeleteDestination(*addr))<0)
		//	UM_LOG_WARN("%s: error in session.DeleteDestination(%s:%u): %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, jrtplib::RTPGetErrorString(status).c_str());
		if((status=_sess.DeleteFromAcceptList(*addr))<0)
			UM_LOG_WARN("%s: error in session.DeleteFromAcceptList(%s:%u): %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, jrtplib::RTPGetErrorString(status).c_str());
		delete addr;
	}
}

void RTPSubscriber::setReceiver(Receiver* receiver) {
	ScopeLock lock(_mutex);
	_receiver=receiver;
}

void RTPSubscriber::run() {
	int status;
	Message *msg;
	while(isStarted()) {
		{
			ScopeLock lock(_mutex);
			if((status=_sess.Poll())<0)
				UM_LOG_WARN("%s: error in session.Poll(): %s", SHORT_UUID(_uuid).c_str(), jrtplib::RTPGetErrorString(status).c_str());
			//TODO: derive own class from rtpsession and override its OnRTPPacket method
			//TODO: maybe handle SR and RR rtcp data packets also and publish this information via metadata in rtp messages send to the user
			if(_receiver!=NULL && (msg=getNextMsg())!=NULL)
				_receiver->receive(msg);
		}
		jrtplib::RTPTime::Wait(jrtplib::RTPTime(1,0));
	}
}

//TODO: inherit this from RTPSession
void RTPSubscriber::OnRTPPacket(jrtplib::RTPPacket *pack, const jrtplib::RTPTime &receivetime, const jrtplib::RTPAddress *senderaddress) {
	assert(pack!=NULL);
	if(_receiver==NULL)
		return;
	Message *msg=new Message((char*)pack->GetPayloadData(), pack->GetPayloadLength(), Message::NONE);
	msg->putMeta("receivedTime", toStr(receivetime.GetDouble()));
	_receiver->receive(msg);
	delete msg;
}

Message* RTPSubscriber::getNextMsg() {
	_sess.BeginDataAccess();
	if(_sess.GotoFirstSourceWithData()) {
		jrtplib::RTPPacket *pack=_sess.GetNextPacket();
		assert(pack!=NULL);
		Message *msg=new Message((char*)pack->GetPayloadData(), pack->GetPayloadLength(), Message::NONE);
		_sess.DeletePacket(pack);
		_sess.EndDataAccess();
		return msg;
	}
	_sess.EndDataAccess();
	return NULL;
}

bool RTPSubscriber::hasNextMsg() {
	bool available;
	_sess.BeginDataAccess();
	available=_sess.GotoFirstSourceWithData();
	_sess.EndDataAccess();
	return available;
}

}