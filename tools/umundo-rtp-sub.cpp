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

class TestReceiver : public Receiver {
public:
	TestReceiver() {};
	void receive(Message* msg) {
		std::string data(msg->data(), msg->size());
		std::cout << "RTP(" << msg->size() << ") --> '" << data << "'" << std::endl << std::flush;
	}
};

int main(int argc, char** argv) {
	printf("umundo-rtp-sub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	TestReceiver testRecv;
	SubscriberConfigRTP subConfig("pingpong");
	//subConfig.setPortbase(42042);
	//subConfig.setMulticastPortbase(42042);
	//subConfig.setMulticastIP("239.1.2.3");		//not needed (default multicast group: 239.8.4.8
	Subscriber subFoo(&subConfig);
	subFoo.setReceiver(&testRecv);
	
	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(subFoo);

	while(1)
		Thread::sleepMs(4000);

	return 0;
}
