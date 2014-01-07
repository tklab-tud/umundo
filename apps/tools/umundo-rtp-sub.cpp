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
public:
	TestReceiver() {};
	void receive(Message* msg) {
		if(msg->getMeta("type")=="RTP")
		{
			char *data=(char*)malloc(msg->size()+1);
			if(data==NULL)
			{
				std::cout << "RTP packet received but error in malloc" << std::endl << std::flush;
				return;
			}
			memcpy(data, msg->data(), msg->size());
			data[msg->size()]='\0';
			std::cout << "RTP(" << msg->size() << ") --> '" << data << "'" << std::endl << std::flush;
			free(data);
		}
		/*else
		{
			std::cout << "RTCP -->";
			std::cout << " fraction='" << msg->getMeta("fraction") << "'";
			std::cout << std::endl << std::flush;
		}*/
	}
};

int main(int argc, char** argv) {
	printf("umundo-rtp-sub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	TestReceiver testRecv;
	RTPSubscriberConfig subConfig;
	Subscriber subFoo(Subscriber::RTP, "pingpong", &testRecv, &subConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(subFoo);

	while(1)
		Thread::sleepMs(4000);

	return 0;
}
