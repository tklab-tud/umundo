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

#include "umundo/connection/rtp/RTPPublisher.h"

namespace umundo {

RTPPublisher::RTPPublisher() : _isSuspended(false), _multicast(false), _initDone(false) {
#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
#endif // WIN32
	_timestamp=libre::rand_u32();
}

void RTPPublisher::init(Options* config) {
	ScopeLock lock(_mutex);
	int status;
	uint16_t portbase=strTo<uint16_t>(config->getKVPs()["pub.rtp.portbase"]);
	_multicastIP=config->getKVPs()["pub.rtp.multicast"];
	_multicastPortbase=strTo<uint16_t>(config->getKVPs()["pub.rtp.multicastPortbase"]);
	double samplesPerSec=strTo<double>(config->getKVPs()["pub.rtp.samplesPerSec"]);
	_timestampIncrement=strTo<uint32_t>(config->getKVPs()["pub.rtp.timestampIncrement"]);
	if(!config->getKVPs().count("pub.rtp.portbase"))
		portbase=16384;
	if(portbase<=0 || portbase>=65536) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid portbase (0 < portbase < 65536)", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(!config->getKVPs().count("pub.rtp.samplesPerSec") || samplesPerSec==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid samplesPerSec (samplesPerSec > 0)", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(!config->getKVPs().count("pub.rtp.timestampIncrement") || _timestampIncrement==0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid timestampIncrement (timestampIncrement > 0)", SHORT_UUID(_uuid).c_str());
		return;
	}
	if(config->getKVPs().count("pub.rtp.multicast") && (!config->getKVPs().count("pub.rtp.multicastPortbase") || _multicastPortbase<=0 || _multicastPortbase>=65536))
	{
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid multicast portbase (0 < portbase < 65536) when using multicast", SHORT_UUID(_uuid).c_str());
		return;
	}

	_payloadType=96;		//dynamic [RFC3551]
	if(config->getKVPs().count("pub.rtp.payloadType"))
		_payloadType=strTo<uint8_t>(config->getKVPs()["pub.rtp.payloadType"]);
	
	/*
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
		UM_LOG_INFO("%s: session.Create() using portbase: %d", SHORT_UUID(_uuid).c_str(), portbase);
	_port=portbase;
	_sess.SetDefaultPayloadType(_payloadType);
	_sess.SetDefaultMark(false);
	_sess.SetDefaultTimestampIncrement(_timestampIncrement);
	
	if(config->getKVPs().count("pub.rtp.multicast")) {
		if(!_sess.SupportsMulticasting())
			UM_LOG_ERR("%s: system not supporting multicast, using unicast", SHORT_UUID(_uuid).c_str());
		else
			_multicast=true;
	}

	start();
	delete transparams;
	*/
	
	_helper=new RTPHelpers();
	struct libre::sa ip;
	libre::sa_init(&ip, AF_INET);
	libre::sa_set_in(&ip, INADDR_ANY, 0);
	//we always have to specify a rtp_recv() handler (so specify an empty function)
	if((status=RTPHelpers::call(boost::bind(libre::rtp_listen, &_rtp_socket, static_cast<int>(IPPROTO_UDP), &ip, 16384, 65535, false, rtp_recv, (void (*)(const libre::sa*, libre::rtcp_msg*, void*)) NULL, this)))) {
		UM_LOG_ERR("%s: error %d in libre::rtp_listen(): %s", SHORT_UUID(_uuid).c_str(), status, strerror(status));
		delete _helper;
		return;
	}
	_port=libre::sa_port(libre::rtp_local(_rtp_socket));
	_initDone=true;
	
	/*struct libre::sa addr;
	libre::sa_init(&addr, AF_INET);
	//net_default_source_addr_get(AF_INET, &addr);
	libre::sa_set_str(&addr, "127.0.0.1", 42042);
	std::cout << addr.len << " port: " << ntohs(addr.u.in.sin_port) << std::endl;
	//addr.len = sizeof(struct sockaddr_in);
	_destinations["127.0.0.1:42042"]=addr;*/
}

RTPPublisher::~RTPPublisher() {
	if(_initDone)
	{
		delete _helper;
		libre::mem_deref(_rtp_socket);
	}
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

	UM_LOG_INFO("%s: received a new subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _channelName.c_str());

	if(_multicast)
	{
		UM_LOG_WARN("%s: configured for multicast, not adding unicast subscriber (%s:%u)", SHORT_UUID(_uuid).c_str(), ip.c_str(), port);
		
		if(_subs.size()==0)		//first subscriber
		{
			UM_LOG_INFO("%s: got first multicast subscriber (%s:%u), adding multicast group %s:%u to receivers list", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _multicastIP.c_str(), _multicastPortbase);
			
			struct libre::sa maddr;
			libre::sa_init(&maddr, AF_INET);
			if((status=libre::sa_set_str(&maddr, _multicastIP.c_str(), _multicastPortbase)))
				UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, _multicastIP.c_str(), _multicastPortbase, strerror(status));
			else
				_destinations[_multicastIP+":"+toStr(_multicastPortbase)]=maddr;
		}
	}
	else
	{
		struct libre::sa addr;
		libre::sa_init(&addr, AF_INET);
		if((status=libre::sa_set_str(&addr, ip.c_str(), port)))
			UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, ip.c_str(), port, strerror(status));
		else
			_destinations[ip+":"+toStr(port)]=addr;
	}

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
	
	UM_LOG_INFO("%s: lost a subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _channelName.c_str());
	
	if(_multicast)
	{
		UM_LOG_WARN("%s: configured for multicast, not removing unicast subscriber (%s:%u)", SHORT_UUID(_uuid).c_str(), ip.c_str(), port);
		
		if(_subs.size()==1)		//last subscriber
		{
			UM_LOG_INFO("%s: lost last multicast subscriber (%s:%u), removing multicast group %s:%u from receivers list", SHORT_UUID(_uuid).c_str(), ip.c_str(), port, _multicastIP.c_str(), _multicastPortbase);
			
			struct libre::sa maddr;
			libre::sa_init(&maddr, AF_INET);
			if((status=libre::sa_set_str(&maddr, _multicastIP.c_str(), _multicastPortbase)))
				UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, _multicastIP.c_str(), _multicastPortbase, strerror(status));
			else
				_destinations.erase(_multicastIP+":"+toStr(_multicastPortbase));
		}
	}
	else
	{
		struct libre::sa addr;
		libre::sa_init(&addr, AF_INET);
		if((status=libre::sa_set_str(&addr, ip.c_str(), port)))
			UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, ip.c_str(), port, strerror(status));
		else
			_destinations.erase(ip+":"+toStr(port));
	}
	
	if (_domainSubs.count(sub.getUUID()) == 1) { // about to vanish
		_subs.erase(sub.getUUID());
	}

	_domainSubs.erase(subIter.first);
	UMUNDO_SIGNAL(_pubLock);
}

void RTPPublisher::send(Message* msg) {
	ScopeLock lock(_mutex);
	int status=0;
	bool marker=strTo<bool>(msg->getMeta("marker"));
	if(!msg->getMeta("marker").size())
		marker=false;
	std::string timestampIncrement=msg->getMeta("timestampIncrement");
	if(!timestampIncrement.size())
		timestampIncrement=_mandatoryMeta["timestampIncrement"];
	if(timestampIncrement.size())
		_timestamp+=strTo<uint32_t>(timestampIncrement);
	else
		_timestamp+=_timestampIncrement;
	
	//allocate buffer
	libre::mbuf *mb = libre::mbuf_alloc(libre::RTP_HEADER_SIZE + msg->size());
	//make room for rtp header
	libre::mbuf_set_end(mb, libre::RTP_HEADER_SIZE);
	libre::mbuf_skip_to_end(mb);
	//write data
	libre::mbuf_write_mem(mb, (const uint8_t*)msg->data(), msg->size());
	//send data
	typedef std::map<std::string, struct libre::sa>::iterator it_type;
	for(it_type iterator = _destinations.begin(); iterator != _destinations.end(); iterator++) {
		std::cout << "pt: " << ((uint32_t)_payloadType) << ", tested: " << (_payloadType&~0x7f) << std::endl;
		std::cout << iterator->second.len << std::endl;
		//reset buffer pos to start of data
		libre::mbuf_set_pos(mb, libre::RTP_HEADER_SIZE);
		if((status=libre::rtp_send(_rtp_socket, &iterator->second, marker, _payloadType, _timestamp, mb)))
			UM_LOG_WARN("%s: error %d in libre::rtp_send() for destination '%s': %s", SHORT_UUID(_uuid).c_str(), status, iterator->first.c_str(), strerror(status));
	}
	//cleanup
	libre::mem_deref(mb);
}

void RTPPublisher::rtp_recv(const struct libre::sa *src, const struct libre::rtp_header *hdr, struct libre::mbuf *mb, void *arg) {
	//do nothing
}

}
