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

#ifndef __has_extension
#define __has_extension __has_feature
#endif

#ifndef WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#endif // WIN32

#include <boost/bind.hpp>

#include "umundo/core/connection/rtp/RTPPublisher.h"

//used for sequence number manipulation (umundo-bridge etc.)
namespace libre {
//taken from libre src/rtp/rtp.c ****** MUST BE UPDATED WHEN LIBRE ITSELF IS UPDATED (libre version used: 0.4.7) ******
/** Defines an RTP Socket */
struct rtp_sock {
	/** Encode data */
	struct {
		uint16_t seq;   /**< Sequence number       */
		uint32_t ssrc;  /**< Synchronizing source  */
	} enc;
	int proto;              /**< Transport Protocol    */
	void *sock_rtp;         /**< RTP Socket            */
	void *sock_rtcp;        /**< RTCP Socket           */
	struct sa local;        /**< Local RTP Address     */
	struct sa rtcp_peer;    /**< RTCP address of Peer  */
	rtp_recv_h *recvh;      /**< RTP Receive handler   */
	rtcp_recv_h *rtcph;     /**< RTCP Receive handler  */
	void *arg;              /**< Handler argument      */
	struct rtcp_sess *rtcp; /**< RTCP Session          */
	bool rtcp_mux;          /**< RTP/RTCP multiplexing */
};
}

namespace umundo {

RTPPublisher::RTPPublisher() : _isSuspended(false), _initDone(false) {
#ifdef WIN32
	WSADATA dat;
	WSAStartup(MAKEWORD(2,2),&dat);
#endif // WIN32

}

void RTPPublisher::init(const Options* config) {
	RScopeLock lock(_mutex);
	int status;
	uint16_t min = 16384;		//minimum rtp port
	uint16_t max = 65534;		//maximum rtp port

	std::map<std::string, std::string> options = config->getKVPs();

	uint16_t portbase   = (options.find("pub.rtp.portbase") !=  options.end() ? strTo<uint16_t>(options["pub.rtp.portbase"]) : 0);
	_timestampIncrement = (options.find("pub.rtp.timestampIncrement") !=  options.end() ? strTo<uint32_t>(options["pub.rtp.timestampIncrement"]) : 0);
	_payloadType        = (options.find("pub.rtp.payloadType") !=  options.end() ? strTo<uint8_t>(options["pub.rtp.payloadType"]) : 96); //dynamic [RFC3551]

	if (options.find("pub.rtp.portbase") != options.end()) { // user did specify portbase
		if((portbase > 0 && portbase < 65534)) {
			min=portbase;
			max=portbase+1;
		} else {
			UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid portbase (0 < portbase < 65535)", SHORT_UUID(_uuid).c_str());
		}
	}

	if (_timestampIncrement == 0) {
		UM_LOG_ERR("%s: error RTPPublisher.init(): you need to specify a valid timestampIncrement (timestampIncrement > 0)", SHORT_UUID(_uuid).c_str());
		return;
	}

	_rtpThread = RTPThread::getInstance();			// this starts the re_main() mainloop...


	struct libre::sa ip;
	libre::sa_init(&ip, AF_INET);
	libre::sa_set_in(&ip, INADDR_ANY, 0);
	//we always have to specify a rtp_recv() handler (so specify an empty function)
	status = _rtpThread->call(boost::bind(libre::rtp_listen,
	                                      &_rtp_socket,
	                                      static_cast<int>(IPPROTO_UDP),
	                                      &ip,
	                                      min,
	                                      max,
	                                      false,
	                                      rtp_recv,
	                                      (void (*)(const libre::sa*, libre::rtcp_msg*, void*))
	                                      NULL,
	                                      this));
	if (status) {
		UM_LOG_ERR("%s: error %d in libre::rtp_listen(): %s", SHORT_UUID(_uuid).c_str(), status, strerror(status));
		return;
	}

	_port           = libre::sa_port(libre::rtp_local(_rtp_socket));
	_timestamp      = libre::rand_u32();
	_sequenceNumber = _rtp_socket->enc.seq;

	libre::udp_sockbuf_set((libre::udp_sock*)libre::rtp_sock(_rtp_socket), 8192*1024);		//try to set something large

	_initDone = true;
}

RTPPublisher::~RTPPublisher() {
	if (_initDone) {
		libre::mem_deref(_rtp_socket);
	}
#ifdef WIN32
	WSACleanup();
#endif // WIN32
}

SharedPtr<Implementation> RTPPublisher::create() {
	return SharedPtr<RTPPublisher>(new RTPPublisher());
}

void RTPPublisher::suspend() {
	//TODO: do something useful on android/ios (e.g. send BYE to subscribers etc.)
	RScopeLock lock(_mutex);
	if (_isSuspended)
		return;
	_isSuspended = true;
};

void RTPPublisher::resume() {
	//TODO: do something useful on android/ios (e.g. send BYE to subscribers etc.)
	RScopeLock lock(_mutex);
	if (!_isSuspended)
		return;
	_isSuspended = false;
};

int RTPPublisher::waitForSubscribers(int count, int timeoutMs) {
	RScopeLock lock(_mutex);
	uint64_t now = Thread::getTimeStampMs();
	while (unique_keys(_domainSubs) < (unsigned int)count) {
		_pubLock.wait(_mutex, timeoutMs);
		if (timeoutMs > 0 && Thread::getTimeStampMs() - timeoutMs > now)
			break;
	}
	return unique_keys(_domainSubs);
}

void RTPPublisher::added(const SubscriberStub& sub, const NodeStub& node) {
	RScopeLock lock(_mutex);
	int status;

	std::string ip = sub.getIP();
	uint16_t port = sub.getPort();
	if (!ip.length())		//only use separate subscriber ip (for multicast support etc.), if specified
		ip = node.getIP();

	// do we already now about this sub via this node?
	std::pair<_domainSubs_t::iterator, _domainSubs_t::iterator> subIter = _domainSubs.equal_range(sub.getUUID());
	while(subIter.first !=  subIter.second) {
		if (subIter.first->second.first.getUUID() ==  node.getUUID())
			return; // we already know about this sub from this node
		subIter.first++;
	}

	UM_LOG_INFO("%s: received a new %s subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), sub.isMulticast() ? "multicast" : "unicast", ip.c_str(), port, _channelName.c_str());

	struct libre::sa addr;
	libre::sa_init(&addr, AF_INET);
	status = libre::sa_set_str(&addr, ip.c_str(), port);

	if (status) {
		UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, ip.c_str(), port, strerror(status));
	} else {
		_destinations[ip + ":" + toStr(port)] = addr;
	}

	_subs[sub.getUUID()] = sub;
	_domainSubs.insert(std::make_pair(sub.getUUID(), std::make_pair(node, sub)));

	if (_greeter !=  NULL && _domainSubs.count(sub.getUUID()) ==  1) {
		// only perform greeting for first occurence of subscriber
		Publisher pub(StaticPtrCast<PublisherImpl>(shared_from_this()));
		_greeter->welcome(pub, sub);
	}

	UMUNDO_SIGNAL(_pubLock);
}

void RTPPublisher::removed(const SubscriberStub& sub, const NodeStub& node) {
	RScopeLock lock(_mutex);
	int status;
	std::string ip = sub.getIP();
	uint16_t port = sub.getPort();
	if (!ip.length())		//only use separate subscriber ip (for multicast support etc.), if specified
		ip = node.getIP();

	// do we now about this sub via this node?
	bool subscriptionFound = false;
	std::pair<_domainSubs_t::iterator, _domainSubs_t::iterator> subIter = _domainSubs.equal_range(sub.getUUID());
	while(subIter.first !=  subIter.second) {
		if (subIter.first->second.first.getUUID() ==  node.getUUID()) {
			subscriptionFound = true;
			break;
		}
		subIter.first++;
	}
	if (!subscriptionFound)
		return;

	UM_LOG_INFO("%s: lost a %s subscriber (%s:%u) for channel %s", SHORT_UUID(_uuid).c_str(), sub.isMulticast() ? "multicast" : "unicast", ip.c_str(), port, _channelName.c_str());

	struct libre::sa addr;
	libre::sa_init(&addr, AF_INET);
	if ((status = libre::sa_set_str(&addr, ip.c_str(), port)))
		UM_LOG_WARN("%s: error %d in libre::sa_set_str(%s:%u): %s", SHORT_UUID(_uuid).c_str(), status, ip.c_str(), port, strerror(status));
	else
		_destinations.erase(ip+":"+toStr(port));

	if (_domainSubs.count(sub.getUUID()) ==  1) { // about to vanish
		if (_greeter !=  NULL) {
			Publisher pub(Publisher(StaticPtrCast<PublisherImpl>(shared_from_this())));
			_greeter->farewell(pub, sub);
		}
		_subs.erase(sub.getUUID());
	}

	_domainSubs.erase(subIter.first);
	UMUNDO_SIGNAL(_pubLock);
}

void RTPPublisher::send(Message* msg) {
	RScopeLock lock(_mutex);
	int status = 0;
	uint32_t timestamp = 0;
	uint8_t payloadType = _payloadType;
	uint16_t sequenceNumber = strTo<uint16_t>(msg->getMeta("um.sequenceNumber"));		//only for internal use by umundo-bridge
	bool marker = strTo<bool>(msg->getMeta("um.marker"));
	if (!msg->getMeta("um.sequenceNumber").size())
		sequenceNumber = _sequenceNumber++;

	if (!msg->getMeta("um.marker").size())
		marker = false;

	std::string timestampIncrement = msg->getMeta("um.timestampIncrement");

	if (!timestampIncrement.size())
		timestampIncrement = _mandatoryMeta["um.timestampIncrement"];

	if (msg->getMeta("um.timestamp").size())	{					//mainly for internal use by umundo-bridge
		timestamp = strTo<uint32_t>(msg->getMeta("um.timestamp"));
	} else {
		if (timestampIncrement.size()) {
			timestamp = (_timestamp += strTo<uint32_t>(timestampIncrement));
		}	else {
			timestamp = (_timestamp += _timestampIncrement);
		}
	}
	if (msg->getMeta("um.payloadType").size())							//mainly for internal use by umundo-bridge
		payloadType = strTo<uint8_t>(msg->getMeta("um.payloadType"));

	//allocate buffer
	libre::mbuf *mb = libre::mbuf_alloc(libre::RTP_HEADER_SIZE + msg->size());
	//make room for rtp header
	libre::mbuf_set_end(mb, libre::RTP_HEADER_SIZE);
	libre::mbuf_skip_to_end(mb);
	//write data
	libre::mbuf_write_mem(mb, (const uint8_t*)msg->data(), msg->size());
	//send data
	typedef std::map<std::string, struct libre::sa>::iterator it_type;
	for(it_type iterator = _destinations.begin(); iterator !=  _destinations.end(); iterator++) {
		//reset buffer pos to start of data
		libre::mbuf_set_pos(mb, libre::RTP_HEADER_SIZE);
		if (sequenceNumber)
			_rtp_socket->enc.seq = sequenceNumber;
		if ((status = libre::rtp_send(_rtp_socket, &iterator->second, marker, payloadType, timestamp, mb)))
			UM_LOG_INFO("%s: error in libre::rtp_send() for destination '%s': %s", SHORT_UUID(_uuid).c_str(), iterator->first.c_str(), strerror(status));
	}
	//cleanup
	libre::mem_deref(mb);
}

void RTPPublisher::rtp_recv(const struct libre::sa *src, const struct libre::rtp_header *hdr, struct libre::mbuf *mb, void *arg) {
	//do nothing
}

}
