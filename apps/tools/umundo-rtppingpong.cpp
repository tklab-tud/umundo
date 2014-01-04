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
#include "umundo/core.h"
#include <iostream>
#include <string.h>

using namespace umundo;

class TestReceiver : public Receiver {
private:
	int i=0;
public:
	TestReceiver() {};
	void receive(Message* msg) {
		char data[msg->size()+1];
		memcpy(data, msg->data(), msg->size());
		data[msg->size()]='\0';
		std::cout << "i(" << msg->size() << ") --> '" << data << "'" << std::endl << std::flush;
		if(i++>8)
			exit(0);
	}
};

int main(int argc, char** argv) {
	printf("umundo-rtppingpong version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	
	if(strcmp(argv[1], "pub")==0)
	{
		RTPPublisherConfig pubConfig(8000, 166, 0);		//PCMU data with sample rate of 8000Hz and 20ms payload per rtp packet (166 samples)
		Publisher pubFoo(Publisher::RTP, "pingpong", &pubConfig);
		
		Discovery disc(Discovery::MDNS);
		Node node;
		disc.add(node);
		node.addPublisher(pubFoo);
		
		while(1) {
			Thread::sleepMs(1000);
			Message* msg = new Message();
			msg->setData("ping", 4);
			std::cout << "o" << std::endl << std::flush;
			pubFoo.send(msg);
			delete(msg);
		}
	}
	
	if(strcmp(argv[1], "sub")==0)
	{
		TestReceiver testRecv;
		RTPSubscriberConfig subConfig;
		Subscriber subFoo(Subscriber::RTP, "pingpong", &testRecv, &subConfig);
		
		Discovery disc(Discovery::MDNS);
		Node node;
		disc.add(node);
		node.addSubscriber(subFoo);
		
		while(1)
			Thread::sleepMs(1000);
	}
	
}
