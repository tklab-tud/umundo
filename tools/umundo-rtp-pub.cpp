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
#include "umundo.h"
#include <iostream>
#include <string.h>

using namespace umundo;

class TestReceiver : public Receiver {
	void receive(Message* msg) {
		std::cout << ".";
	}
};

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
	printf("umundo-rtp-pub version " UMUNDO_VERSION " (" UMUNDO_PLATFORM_ID " " CMAKE_BUILD_TYPE " build)\n");

	PublisherConfigRTP pubConfig("vlc");
	pubConfig.setTimestampIncrement(166); // PCMU data with sample rate of 8000Hz and 20ms payload per rtp packet (166 samples)
	pubConfig.setPayloadType(33);
	Publisher pubFoo(&pubConfig);

	GlobalGreeter greeter;
	pubFoo.setGreeter(&greeter);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);


	Node nodeHack;
	TestReceiver testRcv;
	SubscriberConfigMCast subMCastCfg("vlc");
	subMCastCfg.setMulticastIP("224.1.2.3");
	Subscriber subHack(&subMCastCfg);
	subHack.setReceiver(&testRcv);
	nodeHack.addSubscriber(subHack);
	disc.add(nodeHack);

	// rtp://@224.1.2.3:22020
	pubFoo.waitForSubscribers(1);


	std::cout << "Publisher at 224.1.2.3:" << pubFoo.getPort() << std::endl;

#define MTU 800

	char* readBuffer = (char*) malloc(MTU);

	while(true) {
		std::string file = "/Users/sradomski/Desktop/iShowU-Capture.mov";
		FILE *fp;
		fp = fopen(file.c_str(), "r");
		if (fp == NULL) {
			printf("Failed to open file %s: %s\n", file.c_str(), strerror(errno));
			return EXIT_FAILURE;
		}

		int read = 0;
		int lastread = 0;

		while(true) {
			lastread = fread(readBuffer, 1, MTU, fp);

			if(ferror(fp)) {
				printf("Failed to read from file %s: %s", file.c_str(), strerror(errno));
				return EXIT_FAILURE;
			}

			if (lastread <= 0)
				break;

			pubFoo.send(readBuffer, lastread);
			read += lastread;

			Thread::sleepMs(50);

			if (feof(fp))
				break;
		}
		fclose(fp);

	}

	return 0;
}
