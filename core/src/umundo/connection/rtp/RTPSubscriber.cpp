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

#include <boost/bind.hpp>

#include "umundo/connection/rtp/RTPSubscriber.h"

namespace umundo {

RTPSubscriber::RTPSubscriber() : _extendedSequenceNumber(0), _lastSequenceNumber(0), _multicast(false), _isSuspended(false), _initDone(false) {
#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
#endif // WIN32
}

void RTPSubscriber::init(Options* config) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t min=16384;		//minimum rtp port
	uint16_t max=65535;		//maximum rtp port
	uint16_t portbase=strTo<uint16_t>(config->getKVPs()["sub.rtp.portbase"]);
	_multicastIP=config->getKVPs()["sub.rtp.multicast"];
	if(config->getKVPs().count("pub.rtp.multicast") && !config->getKVPs().count("pub.rtp.portbase")) {
		UM_LOG_ERR("%s: error RTPSubscriber.init(): you need to specify a valid multicast portbase (0 < portbase < 65536) when using multicast", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(config->getKVPs().count("sub.rtp.portbase") && (portbase<=0 || portbase>=65536)) {
		UM_LOG_ERR("%s: error RTPSubscriber.init(): you need to specify a valid portbase (0 < portbase < 65536)", SHORT_UUID(_uuid).c_str());
		return;
	}
	
	/*
	//setting this to zero suppresses RTCP packets (triggering a rtp timeout after some seconds)
	//setting this to one should be reasonable for periodic RTCP receiver reports
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
		UM_LOG_INFO("%s: session.Create() using portbase: %d", SHORT_UUID(_uuid).c_str(), portbase);
	_port=portbase;		//sent to publisher (via framework)
	_sess.SetDefaultPayloadType(_payloadType);
	_sess.SetDefaultMark(false);

	if(config->getKVPs().count("sub.rtp.multicast")) {
		if(!_sess.SupportsMulticasting())
			UM_LOG_ERR("%s: system not supporting multicast, using unicast", SHORT_UUID(_uuid).c_str());
		else
			_multicast=true;
	}

	start();
	delete transparams;
	*/
	
	
	_helper=new RTPHelpers();
	if(config->getKVPs().count("sub.rtp.portbase")) {
		min=portbase;
		max=portbase+1;
	}
	struct libre::sa ip;
	libre::sa_init(&ip, AF_INET);
	libre::sa_set_in(&ip, INADDR_ANY, 0);
	if((status=RTPHelpers::call(boost::bind(libre::rtp_listen, &_rtp_socket, static_cast<int>(IPPROTO_UDP), &ip, min, max, false, rtp_recv, (void (*)(const libre::sa*, libre::rtcp_msg*, void*)) NULL, this)))) {
		UM_LOG_ERR("%s: error %d in libre::rtp_listen()", SHORT_UUID(_uuid).c_str(), status);
		delete _helper;
		return;
	}
	_port=libre::sa_port(libre::rtp_local(_rtp_socket));
	_initDone=true;
}

RTPSubscriber::~RTPSubscriber() {
	stop();
	_cond.broadcast();		//wake up thread
	join();
	if(_initDone)
	{
		delete _helper;
		libre::mem_deref(_rtp_socket);
	}
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
	uint16_t port=pub.getPort();
	std::string ip=node.getIP();

	if(_domainPubs.count(pub.getDomain()) == 0) {
		UM_LOG_INFO("%s: subscribing to %s (%s:%d)", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ip.c_str(), port);

		if(_multicast && _pubs.size()==0) {
			UM_LOG_INFO("%s: first publisher found and we are using multicast, joining multicast group %s:%d now", SHORT_UUID(_uuid).c_str(), _multicastIP.c_str(), _port);
			//TODO: multicast support
		}
	}
	_pubs[pub.getUUID()] = pub;
	_domainPubs.insert(std::make_pair(pub.getDomain(), pub.getUUID()));
}

void RTPSubscriber::removed(const PublisherStub& pub, const NodeStub& node) {
	ScopeLock lock(_mutex);
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
		UM_LOG_INFO("%s unsubscribing from %s (%s:%d)", SHORT_UUID(_uuid).c_str(), pub.getChannelName().c_str(), ip.c_str(), port);

		if(_multicast && _pubs.size()==0) {
			UM_LOG_INFO("%s: last publisher vanished and we are using multicast, leaving multicast group %s:%d now", SHORT_UUID(_uuid).c_str(), _multicastIP.c_str(), _port);
			//TODO: multicast support
		}

	}
}

void RTPSubscriber::setReceiver(Receiver* receiver) {
	ScopeLock lock(_mutex);
	_receiver=receiver;
	if(_receiver)
		start();
}

void RTPSubscriber::run() {
	while(isStarted() && _receiver) {
		//ScopeLock lock(_mutex);
		Message *msg=NULL;
		//_cond.wait(_mutex);
		if(!_receiver || (msg=getNextMsg())==NULL)
			continue;
		_receiver->receive(msg);
		delete msg;
	}
	return;
}

void RTPSubscriber::rtp_recv(const struct libre::sa *src, const struct libre::rtp_header *hdr, struct libre::mbuf *mb, void *arg) {
	RTPSubscriber *obj=(RTPSubscriber*)arg;
	
	if(!mbuf_get_left(mb))
		return;
	
	Message *msg=new Message((char*)mbuf_buf(mb), mbuf_get_left(mb), Message::NONE);
	msg->putMeta("type", "RTP");
	msg->putMeta("marker", toStr((bool)hdr->m));
	msg->putMeta("SSRC", toStr(hdr->ssrc));
	msg->putMeta("timestamp", toStr(hdr->ts));
	msg->putMeta("payloadType", toStr(hdr->pt));
	msg->putMeta("sequenceNumber", toStr(hdr->seq));
	if(hdr->seq<obj->_lastSequenceNumber)		//test for sequence number overflow
		obj->_extendedSequenceNumber++;
	obj->_lastSequenceNumber=hdr->seq;
	msg->putMeta("extendedSequenceNumber", toStr((obj->_extendedSequenceNumber<<16) + hdr->seq));
	msg->putMeta("CSRCCount", toStr(hdr->cc));
	for(int i=0; i<hdr->cc; i++)
		msg->putMeta("CSRC"+toStr(i), toStr(hdr->csrc[i]));
	
	//push new message into queue
	//ScopeLock lock(obj->_mutex);
	obj->_queue.push(msg);
	obj->_cond.broadcast();
}

Message* RTPSubscriber::getNextMsg() {
	if(!hasNextMsg())
		return NULL;
	//ScopeLock lock(_mutex);
	Message *msg=_queue.front();
	_queue.pop();
	return msg;
}

bool RTPSubscriber::hasNextMsg() {
	//ScopeLock lock(_mutex);
	return !_queue.empty();
}

}