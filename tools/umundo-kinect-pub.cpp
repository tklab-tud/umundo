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

Discovery disc;
Node node;
Publisher pubDepth;
Publisher pubVideo;

class FreenectBridge : public Freenect::FreenectDevice {
public:
	FreenectBridge(freenect_context *ctx, int index)
		: Freenect::FreenectDevice(ctx, index), _frameCount(0) {
		this->setLed(LED_RED);
	}

	void VideoCallback(void* image, uint32_t frameTimestamp) {
		return;
	}
	
	void DepthCallback(void* image, uint32_t frameTimestamp) {
		uint16_t *depth = static_cast<uint16_t*>(image);
		uint64_t timestamp = Thread::getTimeStampMs();

		_frameCount++;
		if(!pubDepth || _frameCount % _modulo)			//send every _frameCount modulo _modulo frame
			return;

		std::cout << "Sending depth data (" << (1000/(timestamp-_lastTimestamp)) << " frames per second)..." << std::endl << std::flush;

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
			data->row = i;
			data->timestamp = timestamp;
			
			memcpy(data->data, depth + (640 * i), sizeof(uint16_t) * 640);		//copy one depth data row into rtp data
			msg->setData((char*)data, sizeof(struct RTPData));
			
			pubDepth.send(msg);
			delete data;
			delete msg;
		}
		
		_lastTimestamp=timestamp;
	}

	void setModulo(uint16_t modulo) {
		_modulo=modulo;
	};

private:
	uint16_t _modulo;
	uint8_t _frameCount;
	uint64_t _lastTimestamp;
};

Freenect::Freenect freenect;
FreenectBridge* device;
freenect_video_format requested_format(FREENECT_VIDEO_RGB);

int main(int argc, char** argv) {
	uint16_t modulo=1;

	printf("umundo-kinect-pub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	if(argc>1) {
		int m = strTo<uint16_t>(argv[1]);
		if(m > 0 && m < 256)
			modulo = m;
	}

	std::cout << "Sending every " << modulo << ". image..." << std::endl << std::flush;

	PublisherConfigRTP pubConfig("kinect.depth");
	pubConfig.setTimestampIncrement(1);
	pubDepth = Publisher(&pubConfig);

	disc = Discovery(Discovery::MDNS);
	disc.add(node);

	node.addPublisher(pubDepth);

	int testFrame = 0;
	
	while(true) {
		try {
			device = &freenect.createDevice<FreenectBridge>(0);
			device->setModulo(modulo);
			device->startDepth();
#if 0
	FREENECT_VIDEO_RGB             = 0, /**< Decompressed RGB mode (demosaicing done by libfreenect) */
	FREENECT_VIDEO_BAYER           = 1, /**< Bayer compressed mode (raw information from camera) */
	FREENECT_VIDEO_IR_8BIT         = 2, /**< 8-bit IR mode  */
	FREENECT_VIDEO_IR_10BIT        = 3, /**< 10-bit IR mode */
	FREENECT_VIDEO_IR_10BIT_PACKED = 4, /**< 10-bit packed IR mode */
	FREENECT_VIDEO_YUV_RGB         = 5, /**< YUV RGB mode */
	FREENECT_VIDEO_YUV_RAW         = 6, /**< YUV Raw mode */
	FREENECT_VIDEO_DUMMY           = 2147483647, /**< Dummy value to force enum to be 32 bits wide */
#endif
//			device->setVideoFormat(FREENECT_VIDEO_IR_8BIT);
//			device->startVideo();
			while(1)
				Thread::sleepMs(4000);
			
		} catch(std::runtime_error e) {
			// send a few test images and retry
#if 1
			for (;;) {
				testFrame++;
				uint64_t timestamp = Thread::getTimeStampMs();
				for (unsigned int i = 0; i < 480; i++ ) {
					Message* msg = new Message();
					if (i == 0) {
						msg->putMeta("um.timestampIncrement", toStr(1));
						msg->putMeta("um.marker", toStr(true));
					} else {
						msg->putMeta("um.timestampIncrement", toStr(0));
						msg->putMeta("um.marker", toStr(false));
					}
					struct RTPData *data = new struct RTPData;
					data->row = i;
					data->timestamp = timestamp;

					for (unsigned int j = 0; j < 640; j++) {
						uint16_t tf = (i + j + testFrame);
						tf *= 12;           // frequency
						if (tf > 1200) {   // length of black bar
							tf %= 1200;
						}
						tf += 500;         // start color
						data->data[j] = tf;
					}
					
					msg->setData((char*)data, sizeof(struct RTPData));

					pubDepth.send(msg);

					delete data;
					delete msg;
				}
				
				if (testFrame % 50 == 0)
					break;
				if (testFrame % 640 == 0)
					testFrame = 0;
				Thread::sleepMs(20);
			}
#endif
		}
	}
	

	return 0;
}