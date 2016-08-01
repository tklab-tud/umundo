/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
 */

#include "umundo/config.h"
#include "umundo.h"
#include <string.h>
#include <iostream>
#include <fstream>
#include <exception>

#ifdef WIN32
#include <Winsock2.h>
#include <Iphlpapi.h>
#include <Ws2tcpip.h>
#include "XGetopt.h"
typedef int socklen_t;
#endif

#ifdef UNIX
#include <sys/socket.h>
#include <unistd.h> // gethostname
#include <netdb.h>
#if !defined(ANDROID)
#include <ifaddrs.h>
#endif
#include <netinet/in.h>
#endif

#include <string.h> // strerror
#include <errno.h> // errno
#include <sstream>
#include <iomanip>
#include <stdexcept> // std::runtime_error

#ifdef APPLE
#include <net/if_dl.h> // sockaddr_dl and LLADDR
#endif

#if defined(UNIX) && !defined(APPLE)
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if.h>
#endif

#ifndef SOCKET
#define SOCKET int
#endif

using namespace umundo;

Node node;
SOCKET out;

class SocketException: public std::runtime_error {
public:
	explicit SocketException(const std::string& what) : std::runtime_error(what+": "+strerror(errno)) { }
	virtual ~SocketException() throw () { }
};

std::string ipToStr(uint32_t ip) {
	uint8_t* ip_p=(uint8_t*)&ip;
	std::string output=toStr((int)ip_p[0])+"."+toStr((int)ip_p[1])+"."+toStr((int)ip_p[2])+"."+toStr((int)ip_p[3]);
	return output;
}

void initNetwork(uint16_t port) {
	struct sockaddr_in addr;
	struct sockaddr_in remoteAddress;
	socklen_t remoteAddressLength=sizeof(remoteAddress);
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
	out = socket(PF_INET, SOCK_STREAM, 0);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	//see http://www.microhowto.info/howto/listen_on_a_tcp_port_with_connections_in_the_time_wait_state.html#idp25744
	int reuseaddr = 1;
	if(setsockopt(out, SOL_SOCKET, SO_REUSEADDR, (char*)&reuseaddr, sizeof(reuseaddr))==-1)
		throw SocketException("Could not set SO_REUSEADDR on tcp socket");
	if(bind(out,(struct sockaddr*)&addr, sizeof(addr)) < 0)
		throw SocketException("Could not bind tcp socket to specified port: ");
	if(listen(out, 1)==-1)
		throw SocketException("Could not listen on tcp socket");

	std::cout << "Waiting for connection on *:" << port << "..." << std::endl;
	SOCKET newSocket;
	while(true) {
		newSocket = accept(out, (struct sockaddr*)&remoteAddress, &remoteAddressLength);
		if(newSocket == -1) {
			if(errno == EINTR)
				continue;
			throw SocketException("Could not accept connection on tcp socket");
		}
		break;
	}
#ifdef WIN32
	closesocket(out);
#else
	close(out);
#endif
	out = newSocket;

	std::cout << "Accepted connection from " << ipToStr(remoteAddress.sin_addr.s_addr) << ":" << ntohs(remoteAddress.sin_port) << "..." << std::endl;
	char buffer[4096];
	Thread::sleepMs(4000);
	recvfrom(out, buffer, sizeof(buffer), 0, NULL, 0);
	char header[]="HTTP/1.1 200 OK\nServer: umundo-stream-receive\nX-Powered-By: umundo/" UMUNDO_VERSION "\n\n";
	send(out, header, sizeof(header), 0);
	std::cout << "Send HTTP headers to " << ipToStr(remoteAddress.sin_addr.s_addr) << ":" << ntohs(remoteAddress.sin_port) << "..." << std::endl;
}

class TestReceiver : public Receiver {
private:
	uint32_t len;
public:
	TestReceiver() : len(0) {};
	void receive(Message* msg) {
		send(out, msg->data(), msg->size(), 0);
		len+=msg->size();
		if(len>1024*1024) {			//output i every 1MB
			std::cout << "i" << std::flush;
			len=0;
		}
	}
};

int main(int argc, char** argv) {
	printf("umundo-stream-receive version " UMUNDO_VERSION " (" UMUNDO_PLATFORM_ID " " CMAKE_BUILD_TYPE " build)\n");

	initNetwork(5040);

	TestReceiver* testRecv = new TestReceiver();
	Subscriber subFoo("stream");
	subFoo.setReceiver(testRecv);

	Discovery disc(Discovery::MDNS);
	disc.add(node);
	node.addSubscriber(subFoo);

	while(1)
		Thread::sleepMs(4000);
}
