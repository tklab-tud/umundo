/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#include "umundo/common/Host.h"
#include "umundo/config.h"

#ifdef WIN32
#include <Winsock2.h>
#include <Iphlpapi.h>
#endif

#ifdef UNIX
#include <sys/socket.h>
#if !defined(ANDROID)
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#endif

#include <string.h> // strerror
#include <errno.h> // errno
#include <sstream>
#include <iomanip>

#ifdef APPLE
#include <net/if_dl.h> // sockaddr_dl and LLADDR
#endif

#if defined(UNIX) && !defined(APPLE)
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#if 0
#include <arpa/inet.h>
#include <net/if.h>
#endif

namespace umundo {

#ifdef WIN32
# define MAX_HOST_NAME_LENGTH 255
#else
# define MAX_HOST_NAME_LENGTH 1024
#endif

std::string hostId;
std::string hostName;

const std::string Host::getHostname() {
	int err;
	char* name = (char*)calloc(MAX_HOST_NAME_LENGTH, 1);

#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

	err = gethostname(name, MAX_HOST_NAME_LENGTH);
	if (err != 0) {

#if defined(UNIX) || defined(IOS)
		UM_LOG_ERR("gethostname: %s", strerror(errno));
#elif defined(WIN32)
		switch(err) {
		case WSAEFAULT:
			UM_LOG_ERR("gethostname: The name parameter is a NULL pointer");
			break;
		case WSANOTINITIALISED:
			UM_LOG_ERR("gethostname: No prior successful WSAStartup call");
			break;
		case WSAENETDOWN:
			UM_LOG_ERR("gethostname: The network subsystem has failed");
			break;
		case WSAEINPROGRESS:
			UM_LOG_ERR("gethostname: A blocking Windows Sockets 1.1 call is in progress");
			break;
		default:
			UM_LOG_ERR("gethostname: returned unknown error?!");
			break;
		}
		// TODO: is this needed?
		//WSACleanup();
#else
#error "Don't know how to get the hostname on this platform"
#endif
		hostName = "";
	} else {
		hostName = name;
	}
	free(name);
	return hostName;
}

const std::vector<Interface> Host::getInterfaces() {
	std::vector<Interface> ifcs;
	int err = 0;
	(void)err;

#if defined(UNIX) && !defined(ANDROID)
	struct ifaddrs *ifaddr;
	err = getifaddrs(&ifaddr);
	if (err != 0) {
		UM_LOG_ERR("getifaddrs: %s", strerror(errno));
		return ifcs;
	}

# ifdef SIOCGIFHWADDR
	struct ifreq ifinfo;
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		UM_LOG_ERR("socket: %s", strerror(errno));
		return ifcs;
	};
# endif

	struct ifaddrs *ifa = ifaddr;

	// Search for the first device with an ip and a mac
	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		Interface currIfc;
		currIfc.name = ifa->ifa_name;

# ifdef SIOCGIFHWADDR
		strcpy(ifinfo.ifr_name, ifa->ifa_name);
		err = ioctl(sock, SIOCGIFHWADDR, &ifinfo);
		if (err == 0) {
			if (ifinfo.ifr_hwaddr.sa_family == 1) {
				currIfc.mac = std::string(ifinfo.ifr_hwaddr.sa_data, IFHWADDRLEN);
			}
		} else {
			UM_LOG_ERR("ioctl: %s", strerror(errno));
		}
# endif
		if (ifa->ifa_addr != NULL) {
			int family = ifa->ifa_addr->sa_family;
			switch (family) {
			case AF_INET:
				currIfc.ipv4.push_back(std::string((char*)&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr, 4));
				break;
			case AF_INET6:
				currIfc.ipv4.push_back(std::string((char*)&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr, 6));
				break;
# ifdef LLADDR
			case AF_LINK:
				struct sockaddr_dl* sdl = (struct sockaddr_dl *)ifa->ifa_addr;
				if (sdl->sdl_alen == 6) {
					currIfc.mac = std::string(LLADDR(sdl), sdl->sdl_alen);
				}
				break;
# endif
			}
			ifcs.push_back(currIfc);
		}
	}
	freeifaddrs(ifaddr);
#endif

#ifdef WIN32
	// from http://www.codeguru.com/cpp/i-n/network/networkinformation/article.php/c5451/Three-ways-to-get-your-MAC-address.htm
	IP_ADAPTER_INFO AdapterInfo[256];
	DWORD dwBufLen = sizeof(AdapterInfo);
	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS) {
		UM_LOG_ERR("GetAdaptersInfo returned with error");
		return ifcs;
	}

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	do {
		Interface currIfc;

		if (pAdapterInfo->AddressLength == 6)
			currIfc.mac = std::string((const char*)pAdapterInfo->Address, 6);

		currIfc.name = std::string(pAdapterInfo->AdapterName);
		pAdapterInfo = pAdapterInfo->Next;
		ifcs.push_back(currIfc);
	} while(pAdapterInfo);
#endif

#ifdef ANDROID
	// TODO: Update when there actually is a way to get the MAC
#endif

	return ifcs;
}


const std::string Host::getHostId() {

	if (hostId.size() > 0)
		return hostId;

	std::string hostname = getHostname();
	std::vector<Interface> interfaces = getInterfaces();

	std::ostringstream ss;
	ss << std::hex << std::uppercase << std::setfill( '0' );

	// first all the mac adresses
	std::vector<Interface>::iterator ifIter = interfaces.begin();
	while(ifIter != interfaces.end()) {
		if (ifIter->mac.length() > 0)
			for (int i = 0; i < ifIter->mac.length(); i++)
				ss << std::setw( 2 ) << (int)ifIter->mac[i];
		ifIter++;
	}

	// next the hostname
	if (ss.str().length() < 36 && hostname.length() > 0) {
		for (unsigned int i = 0; i < hostname.length(); i++)
			ss << std::setw( 2 ) << (int)hostname[i];
	}

	// append ipv4 addresses as well
	if (ss.str().length() < 36) {
		ifIter = interfaces.begin();
		while(ifIter != interfaces.end()) {
			std::vector<std::string>::iterator ipIter = ifIter->ipv4.begin();
			while(ipIter != ifIter->ipv4.end()) {
				if (ipIter->length() > 0)
					for (int i = 0; i < ipIter->length(); i++)
						ss << std::setw( 2 ) << (int)(*ipIter)[i];
				ipIter++;
			}
			ifIter++;
		}
	}

	// and ipv6 addresses
	if (ss.str().length() < 36) {
		ifIter = interfaces.begin();
		while(ifIter != interfaces.end()) {
			std::vector<std::string>::iterator ipIter = ifIter->ipv6.begin();
			while(ipIter != ifIter->ipv6.end()) {
				if (ipIter->length() > 0)
					for (int i = 0; i < ipIter->length(); i++)
						ss << std::setw( 2 ) << (int)(*ipIter)[i];
				ipIter++;
			}
			ifIter++;
		}
	}

	// padding
	if (ss.str().length() < 36)
		for (int i = 0; i < 36 - ss.str().length(); i++)
			ss << std::setw( 2 ) << 0;


	hostId = ss.str().substr(0, 36);
	return hostId;
}

}
