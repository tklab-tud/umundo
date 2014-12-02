/**
 *  Copyright (C) 2013  Thilo Molitor (thilo@eightysoft.de)
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
#include "umundo/core.h"
#include <iostream>
#include <string.h>

using namespace umundo;

class GlobalGreeter : public Greeter {
public:
	GlobalGreeter() { }
	void welcome(Publisher& pub, const SubscriberStub& subStub) {
		std::cout << "Got new LOCAL " << (subStub.isMulticast() ? "multicast" : "unicast") << " " << (subStub.isRTP() ? "RTP" : "ZMQ") << " subscriber: " << subStub << std::endl;
	}
	
	void farewell(Publisher& pub, const SubscriberStub& subStub) {
		std::cout << "Removed LOCAL " << (subStub.isMulticast() ? "multicast" : "unicast") << " " << (subStub.isRTP() ? "RTP" : "ZMQ") << " subscriber: " << subStub << std::endl;
	}
};

int main(int argc, char** argv) {
	printf("umundo-rtp-pub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	
	RTPPublisherConfig pubConfig(166, 0);		//PCMU data with sample rate of 8000Hz and 20ms payload per rtp packet (166 samples)
	Publisher pubFoo(Publisher::RTP, "pingpong", new GlobalGreeter(), &pubConfig);
	
	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);
	
	uint16_t num=0;
	char buf[2]="A";
	while(1) {
		Thread::sleepMs(1000);
		Message* msg = new Message();
		buf[0]=65+num++;
		if(num==26)
			num=0;
		std::string ping=std::string("ping-")+std::string(buf);
		msg->setData(ping.c_str(), ping.length());
		std::cout << "o-" << buf << std::endl << std::flush;
		pubFoo.send(msg);
		delete(msg);
	}

	return 0;
}
