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

#include <libfreenect.hpp>

struct RTPData {
	uint16_t row;
	uint64_t timestamp;
	uint16_t data[640];
};

using namespace umundo;

/* thanks to Yoda---- from IRC */
class MyFreenectDevice : public Freenect::FreenectDevice {
public:
	MyFreenectDevice(freenect_context *ctx, int index)
		: Freenect::FreenectDevice(ctx, index), _pub(NULL), _frameCount(0) {
		this->setLed(LED_RED);
	}
	// Do not call directly even in child
	void VideoCallback(void* _rgb, uint32_t timestamp) {
		return;
	}
	// Do not call directly even in child
	void DepthCallback(void* depthData, uint32_t frameTimestamp) {
		uint16_t *depth = static_cast<uint16_t*>(depthData);
		uint64_t timestamp = Thread::getTimeStampMs();

		_frameCount++;
		std::cout << "got new depth data (kinect timestamp: " << frameTimestamp << ", modulo: " << _frameCount%_modulo << ")";
		if(!(_frameCount%_modulo)) {
			std::cout << ", sending data (" << (1000/(timestamp-_lastTimestamp)) << " frames per second)...";
		}
		std::cout << std::endl << std::flush;
		if(!_pub || _frameCount%_modulo)			//send every _frameCount modulo _modulo frame
			return;

		for( unsigned int i = 0; i < 480; i++ ) {
			Message* msg = new Message();
			//set marker to indicate new frame start and timestampIncrement so that individual rtp packets for one frame share the same rtp timestamp
			if(i==0) {
				msg->putMeta("um.timestampIncrement", toStr(1));
				msg->putMeta("um.marker", toStr(true));
			} else {
				msg->putMeta("um.timestampIncrement", toStr(0));
				msg->putMeta("um.marker", toStr(false));
			}
			struct RTPData *data = new struct RTPData;
			data->row=i;
			data->timestamp=timestamp;
			memcpy(data->data, depth+(640*i), sizeof(uint16_t)*640);		//copy one depth data row into rtp data
			msg->setData((char*)data, sizeof(struct RTPData));
			_pub->send(msg);
			delete data;
			delete msg;
		}
		_lastTimestamp=timestamp;
	}
	void setPub(Publisher *pub) {
		_pub=pub;
	};

	void setModulo(uint16_t modulo) {
		_modulo=modulo;
	};

private:
	Publisher *_pub;
	uint16_t _modulo;
	uint8_t _frameCount;
	uint64_t _lastTimestamp;
};

Freenect::Freenect freenect;
MyFreenectDevice* device;
freenect_video_format requested_format(FREENECT_VIDEO_RGB);

int main(int argc, char** argv) {
	uint16_t modulo=1;

	printf("umundo-kinect-pub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	if(argc>1) {
		int m=strTo<uint16_t>(argv[1]);
		if(m>0 && m<256)
			modulo=m;
	}

	std::cout << "Sending every " << modulo << ". image..." << std::endl << std::flush;

	PublisherConfigRTP pubConfig("kinect-pubsub");
	pubConfig.setTimestampIncrement(1);
	Publisher pubFoo(&pubConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);

	device = &freenect.createDevice<MyFreenectDevice>(0);
	device->setPub(&pubFoo);
	device->setModulo(modulo);
	device->startDepth();

	while(1)
		Thread::sleepMs(4000);

	return 0;
}