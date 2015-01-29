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

#define MAX_PAYLOAD_PACKET 1400

struct RTPDepthData {
	uint16_t row;
	uint64_t timestamp;
	uint16_t data[640];
};

struct RTPVideoData {
	uint16_t row;
	uint64_t timestamp;
	uint8_t segment;    // first four bit signify total segments, last four this segments index in big endian
	uint8_t data[MAX_PAYLOAD_PACKET]; // largest data that will reasonable fit
};

using namespace umundo;

Discovery disc;
Node node;
Publisher pubDepthTCP;
Publisher pubDepthRTP;

Publisher pubVideoTCP;
Publisher pubVideoRTP;

class FreenectBridge : public Freenect::FreenectDevice {
public:
	FreenectBridge(freenect_context *ctx, int index) : Freenect::FreenectDevice(ctx, index), _frameCountDepth(0), _frameCountVideo(0) {
		this->setLed(LED_RED);
	}

	void VideoCallback(void* frame, uint32_t frameTimestamp) {
		uint8_t* rgb = static_cast<uint8_t*>(frame);
		uint64_t timestamp = Thread::getTimeStampMs();

		uint16_t rows = 0;
		uint16_t cols = 0;

		if (_frameCountVideo % 30 == 0)
			std::cout << "Video data @" << (1000/(timestamp-_lastTimestampVideo)) << "fps" << std::endl << std::flush;

		_frameCountVideo++;
		if(_frameCountVideo % _modulo)			//send every _frameCountDepth modulo _modulo frame
			return;
		
		switch (getVideoResolution()) {
			case FREENECT_RESOLUTION_LOW:
				cols = 320;
				rows = 240;
				break;
			case FREENECT_RESOLUTION_MEDIUM:
				cols = 640;
				rows = 480;
				break;
			case FREENECT_RESOLUTION_HIGH:
				cols = 1280;
				rows = 1024;
				break;
			default:
    break;
		}

		size_t frameSize = getVideoBufferSize();
		size_t rowSize = (frameSize / rows);
		size_t dataSize = rowSize;
		size_t segments = 1;
		
		while(dataSize > MAX_PAYLOAD_PACKET) {
			dataSize /= 2;
			segments *= 2;
		}
		
		if (_frameCountVideo % (_modulo * 5) == 0)
			std::cout << "Row: " << rowSize << "B" << " => Data: " << dataSize << "B in " << segments << " segments" << std::endl << std::flush;

		for (size_t row = 0; row < rows; row++) {
			for (size_t segment = 0; segment < segments; segment++) {
				Message* tcpMsg = new Message();

				if (row == 0 && segment == 0) {
					tcpMsg->putMeta("um.timestampIncrement", toStr(1));
					tcpMsg->putMeta("um.marker", toStr(true));
				} else {
					tcpMsg->putMeta("um.timestampIncrement", toStr(0));
					tcpMsg->putMeta("um.marker", toStr(false));
				}

				struct RTPVideoData *data = new struct RTPVideoData;
				data->row = row;
				data->timestamp = timestamp;
				data->segment = 0;
				data->segment += segments << 4; // total segments
				data->segment += segment;       // current segment
				
				// copy one segmented row into data
				memcpy(data->data, rgb + (cols * row + dataSize), dataSize);		//copy one depth data row into rtp data
				tcpMsg->setData((char*)data, sizeof(struct RTPVideoData) - (MAX_PAYLOAD_PACKET - dataSize));
				
				Message* rtpMsg = new Message(*tcpMsg); // we need to copy as the publisher will add meta fields! TODO: change this
				
				pubVideoTCP.send(tcpMsg);
				pubVideoRTP.send(rtpMsg);

				delete data;
				delete tcpMsg;
				delete rtpMsg;
			}
		}
		_lastTimestampVideo = timestamp;
	}
	
	
	void DepthCallback(void* frame, uint32_t frameTimestamp) {
		uint16_t *depth = static_cast<uint16_t*>(frame);
		uint64_t timestamp = Thread::getTimeStampMs();

		if (_frameCountDepth % 30 == 0)
			std::cout << "Depth data @" << (1000/(timestamp-_lastTimestampDepth)) << "fps" << std::endl << std::flush;

		_frameCountDepth++;
		if(_frameCountDepth % _modulo)			//send every _frameCountDepth modulo _modulo frame
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
			
			struct RTPDepthData *data = new struct RTPDepthData;
			data->row = i;
			data->timestamp = timestamp;
			
			memcpy(data->data, depth + (640 * i), sizeof(uint16_t) * 640);		//copy one depth data row into rtp data
			msg->setData((char*)data, sizeof(struct RTPDepthData));
			
			pubDepthTCP.send(msg);
			pubDepthRTP.send(msg);

			delete data;
			delete msg;
		}
		_lastTimestampDepth = timestamp;
	}

	void setModulo(uint16_t modulo) {
		_modulo=modulo;
	};

private:
	uint16_t _modulo;
	uint16_t _frameCountDepth;
	uint16_t _frameCountVideo;
	uint64_t _lastTimestampDepth;
	uint64_t _lastTimestampVideo;
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

	std::cout << "Sending every " << modulo << ". frame..." << std::endl << std::flush;

	{
		PublisherConfigRTP pubConfigRTP("kinect.depth.rtp");
		pubConfigRTP.setTimestampIncrement(1);
		pubDepthRTP = Publisher(&pubConfigRTP);

		PublisherConfigTCP pubConfigTCP("kinect.depth.tcp");
		pubDepthTCP = Publisher(&pubConfigTCP);
	}
	
	{
		PublisherConfigRTP pubConfigRTP("kinect.video.rtp");
		pubConfigRTP.setTimestampIncrement(1);
		pubVideoRTP = Publisher(&pubConfigRTP);

		PublisherConfigTCP pubConfigTCP("kinect.video.tcp");
		pubVideoTCP = Publisher(&pubConfigTCP);
	}
	
	disc = Discovery(Discovery::MDNS);
	disc.add(node);

	node.addPublisher(pubDepthRTP);
	node.addPublisher(pubDepthTCP);
	node.addPublisher(pubVideoRTP);
	node.addPublisher(pubVideoTCP);

	while(true) {
		try {
			device = &freenect.createDevice<FreenectBridge>(0);
			device->setModulo(modulo);
			device->startDepth();
//			device->setVideoFormat(FREENECT_VIDEO_RGB, FREENECT_RESOLUTION_MEDIUM);
			device->startVideo();
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
			std::cout << e.what() << std::endl;
			Thread::sleepMs(4000);
			// send a few test frames and retry
		}
	}
	

	return 0;
}