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
#include <cmath>
#include <vector>

#include <libfreenect.hpp>

using namespace umundo;

uint16_t modulo=1;

/* thanks to Yoda---- from IRC */
class MyFreenectDevice : public Freenect::FreenectDevice {
public:
	MyFreenectDevice(freenect_context *_ctx, int _index)
		: Freenect::FreenectDevice(_ctx, _index), m_buffer_depth(freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB).bytes), m_gamma(2048), pubFoo(NULL), count(0)
	{
		for( unsigned int i = 0 ; i < 2048 ; i++) {
			float v = i/2048.0;
			v = std::pow(v, 3)* 6;
			m_gamma[i] = v*6*256;
		}
		this->setLed(LED_RED);
	}
	//~MyFreenectDevice(){}
	// Do not call directly even in child
	void VideoCallback(void* _rgb, uint32_t timestamp) {
		return;
	}
	// Do not call directly even in child
	void DepthCallback(void* _depth, uint32_t timestamp) {
		count++;
		std::cout << "got new depth data (timestamp: " << timestamp << ", modulo: " << count%modulo << ")";
		if(!(count%modulo))
			std::cout << ", sending data...";
		std::cout << std::endl << std::flush;
		if(!pubFoo || count%modulo)
			return;
		uint16_t* depth = static_cast<uint16_t*>(_depth);
		uint16_t buffer[640*480];
		for( unsigned int i = 0 ; i < 640*480 ; i++) {
			uint16_t pval = m_gamma[depth[i]];
			buffer[i]=pval;
		}
		for( unsigned int i = 0; i < 480; i++ ) {
			Message* msg = new Message();
			msg->putMeta("timestampIncrement", toStr(0));
			msg->putMeta("marker", toStr(false));
			if(i==479)
				msg->putMeta("timestampIncrement", toStr(1));
			if(i==0)
				msg->putMeta("marker", toStr(true));
			msg->setData((char*)(buffer+(640*i)), sizeof(uint16_t)*640);
			pubFoo->send(msg);
			delete(msg);
			//sleep some time to give network buffers time to send out our data (this helps against periodic packet loss)
			Thread::sleepNs(128000);
		}
	}
	void setPub(Publisher *pub) {
		this->pubFoo=pub;
	};

private:
	std::vector<uint8_t> m_buffer_depth;
	std::vector<uint16_t> m_gamma;
	Publisher *pubFoo;
	uint8_t count;
};

Freenect::Freenect freenect;
MyFreenectDevice* device;
freenect_video_format requested_format(FREENECT_VIDEO_RGB);

int main(int argc, char** argv) {
	printf("umundo-kinect-pub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	if(argc>1) {
		int m=strTo<uint16_t>(argv[1]);
		if(m>0 && m<256)
			modulo=m;
	}
	
	std::cout << "Sending every " << modulo << ". image..." << std::endl << std::flush;
	
	RTPPublisherConfig pubConfig(1, 1);
	Publisher pubFoo(Publisher::RTP, "kinect-pubsub", &pubConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);

	device = &freenect.createDevice<MyFreenectDevice>(0);
	device->setPub(&pubFoo);
	device->startDepth();

	while(1)
		Thread::sleepMs(4000);

	return 0;
}
