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

int main(int argc, char** argv) {
	printf("umundo-rtp-pub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	RTPPublisherConfig pubConfig(8000, 166, 0);		//PCMU data with sample rate of 8000Hz and 20ms payload per rtp packet (166 samples)
	Publisher pubFoo(Publisher::RTP, "pingpong", &pubConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);

	while(1) {
		Thread::sleepMs(100);
		Message* msg = new Message();
		msg->setData("ping", 4);
		std::cout << "o" << std::endl << std::flush;
		pubFoo.send(msg);
		delete(msg);
	}

	return 0;
}
