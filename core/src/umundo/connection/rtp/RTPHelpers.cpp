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

#include "umundo/connection/rtp/RTPHelpers.h"

namespace umundo {

RTPHelpers::RTPHelpers() {}

RTPHelpers::~RTPHelpers() {}

jrtplib::RTPAddress *RTPHelpers::strToAddress(bool isIPv6, std::string ip, uint16_t port) {
	if(isIPv6) {
		struct sockaddr_in6 sa;
		inet_pton(AF_INET6, ip.c_str(), &(sa.sin6_addr));
		return new jrtplib::RTPIPv6Address((const uint8_t*)&(sa.sin6_addr), port);
	} else {
		struct sockaddr_in sa;
		inet_pton(AF_INET, ip.c_str(), &(sa.sin_addr));
		return new jrtplib::RTPIPv4Address((const uint8_t*)&(sa.sin_addr), port);
	}
}

}