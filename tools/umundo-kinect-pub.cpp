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

	void VideoCallback(void* raw, uint32_t frameTimestamp) {
		uint8_t* frame = static_cast<uint8_t*>(raw);
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
		std::vector<uint8_t> scaled(frameSize / 4);

		/**
		 * naive rescaling for bayes
		 * Pixel arrangement:
		 *
		 * G R G R G R G R
		 * B G B G B G B G
		 * G R G R G R G R
		 * B G B G B G B G
		 * G R G R G R G R
		 * B G B G B G B G
		 */
		
		size_t blockRows = 240; // top to bottom 240 -> 120 blocks
		size_t blockCols = 320; // left to right 320 -> 160 blocks
		size_t hereRight = 1;
		size_t hereDown = 320;
		size_t thereRight = 1;
		size_t thereDown = 640;
		size_t maxHere = 0;
		
		for (int y = 0; y < blockRows; y += 2) {
			for (int x = 0; x < blockCols; x += 2) {
				// we need to allocate a 2x2 block each
				size_t here  = x + (y * blockCols);
				size_t there = 2 * x + (4 * y * blockCols);
				assert(here < 320*240 - 320);
				assert(there < 640*480 - 640);
				scaled[here]                        = frame[there];
				scaled[here + hereRight]            = frame[there + thereRight];
				scaled[here + hereDown]             = frame[there + thereDown];
				scaled[here + hereRight + hereDown] = frame[there + thereRight + thereDown];
				if (here > maxHere)
					maxHere = here;
			}
		}

		// publish complete frame per tcp, publisher will compress
		{
			Message tcpMsg;
			tcpMsg.putMeta("um.marker", toStr(true));
			tcpMsg.setData((const char*)&scaled[0], scaled.size());

			
#if 1
			// shave off last few bits to help compression
			char* msgData = const_cast<char*>(tcpMsg.data());
			for (int i = 0; i < tcpMsg.size(); i++) {
//				msgData[i] &= 0xFC;
				msgData[i] &= 0xE0;
//					msgData[i] &= 0xF0;
			}
#endif
			
			std::cout << tcpMsg.size() << " -> ";
			tcpMsg.compress();
			std::cout << tcpMsg.size() << std::endl;
			
			pubVideoTCP.send(&tcpMsg);
		}
		
		{
			// chop into RTP packets
			size_t index = 0;
			while (index < scaled.size()) {
				Message rtpMsg;
				if (index == 0) {
					rtpMsg.putMeta("um.timestampIncrement", toStr(1));
					rtpMsg.putMeta("um.marker", toStr(true));
				} else {
					rtpMsg.putMeta("um.timestampIncrement", toStr(0));
					rtpMsg.putMeta("um.marker", toStr(false));
				}
				
				char* buffer = (char*)malloc(MAX_PAYLOAD_PACKET + 2);
				Message::write((uint16_t)index, buffer);
				memcpy(&buffer[2], &scaled[index], index + MAX_PAYLOAD_PACKET > scaled.size() ? scaled.size() - index : MAX_PAYLOAD_PACKET);
				rtpMsg.setData(buffer, MAX_PAYLOAD_PACKET + 2);
				free(buffer);
//				Thread::sleepUs(800);
				pubVideoRTP.send(&rtpMsg);

				index += MAX_PAYLOAD_PACKET;
			}
		}

		_lastTimestampVideo = timestamp;
	}
	
	
	void DepthCallback(void* frame, uint32_t frameTimestamp) {
		return;
#if 0
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
			
//			pubDepthTCP.send(msg);
			pubDepthRTP.send(msg);

			delete data;
			delete msg;
		}
		_lastTimestampDepth = timestamp;
#endif
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
		pubConfigTCP.enableCompression();
		pubDepthTCP = Publisher(&pubConfigTCP);
	}
	
	{
		PublisherConfigRTP pubConfigRTP("kinect.video.rtp");
		pubConfigRTP.setTimestampIncrement(1);
		pubVideoRTP = Publisher(&pubConfigRTP);

		PublisherConfigTCP pubConfigTCP("kinect.video.tcp");
		pubConfigTCP.enableCompression();
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
			device->setVideoFormat(FREENECT_VIDEO_BAYER, FREENECT_RESOLUTION_MEDIUM);
//			device->setVideoFormat(FREENECT_VIDEO_RGB,   FREENECT_RESOLUTION_HIGH);
//			device->setDepthFormat(FREENECT_DEPTH_11BIT, FREENECT_RESOLUTION_HIGH);

			device->startDepth();
			device->startVideo();
#if 0
	FREENECT_VIDEO_RGB             = 0, /**< Decompressed RGB mode (demosaicing done by libfreenect)  921600B/frame */
	FREENECT_VIDEO_BAYER           = 1, /**< Bayer compressed mode (raw information from camera)      307200B/frame */
	FREENECT_VIDEO_IR_8BIT         = 2, /**< 8-bit IR mode                                            312320B/frame */
	FREENECT_VIDEO_IR_10BIT        = 3, /**< 10-bit IR mode                                           624640B/frame */
	FREENECT_VIDEO_IR_10BIT_PACKED = 4, /**< 10-bit packed IR mode                                    390400B/frame */
	FREENECT_VIDEO_YUV_RGB         = 5, /**< YUV RGB mode                                             921600B/frame */
	FREENECT_VIDEO_YUV_RAW         = 6, /**< YUV Raw mode                                             614400B/frame */
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