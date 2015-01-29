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
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <cmath>
#include <vector>

#if defined(__APPLE__)
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glu.h>
#endif

struct RTPDepthData {
	uint16_t row;
	uint64_t timestamp;
	uint16_t data[640];
};

struct RTPVideoData {
	uint16_t row;
	uint16_t cols;
	uint64_t timestamp;
	uint8_t data[640]; // may not be completely used with other resolutions
};

using namespace umundo;

GLuint gl_depth_tex;

class VideoReceiver : public Receiver {
public:
	VideoReceiver() : _rtpTimestamp(0), _start(false), _timeOffset(0), _videoBuffer(640*480*4) {
	}
	
	~VideoReceiver() {
	}
	
	void receive(Message* msg) {
		//handle only RTP messages
		if (msg->getMeta("um.type") != "RTP")
			return;
		
		RScopeLock lock(_internalVideoMutex);
		bool marker = strTo<bool>(msg->getMeta("um.marker"));
		struct RTPVideoData* data = (struct RTPVideoData*)msg->data();
		
		//calculate time offset to remote host
		if (!_timeOffset)
			_timeOffset = Thread::getTimeStampMs() - data->timestamp;

		if (marker) {
			std::cout << std::endl << "####" << std::endl;
			_rtpTimestamp = strTo<uint32_t>(msg->getMeta("um.timestamp"));
			if (_start) {
				_newCompleteFrame = true;
				UMUNDO_SIGNAL(_newFrame);
			}
			_start = true;
		}
		
		_lastRemoteTimestamp = data->timestamp;
		_lastLocalTimestamp = Thread::getTimeStampMs() - _timeOffset;
		
		if (!_start)				//wait for first complete frame (and ignore data till then)
			return;

		std::cout << data->row << ":" << msg->size() << ".";
//		memcpy(&_videoBuffer[data->row * data->cols], data->data, sizeof(uint8_t) * data->cols);
		memset(&_videoBuffer[data->row * data->cols * 4], 200, sizeof(uint8_t) * data->cols * 4);
	}
	
	bool getRGB(std::vector<uint8_t> &buffer) {
		RScopeLock lock(_internalVideoMutex);
		_newFrame.wait(_internalVideoMutex, 100);

		if (!_newCompleteFrame)
			return false;
		
		buffer.swap(_videoBuffer);
		_newCompleteFrame = false;

		return true;
	}

protected:
	uint32_t _rtpTimestamp;
	bool _newCompleteFrame;
	bool _start;
	uint64_t _lastRemoteTimestamp;
	uint64_t _lastLocalTimestamp;

	int64_t _timeOffset;

	RMutex _internalVideoMutex;
	Monitor _newFrame;

	std::vector<uint8_t> _videoBuffer;

};

class DepthReceiver : public Receiver {
public:
	DepthReceiver() : _mGamma(2048), _rtpTimestamp(0), _start(false), _timeOffset(0), _depthBuffer(640*480*3) {
		for (unsigned int i = 0; i<480; i++)
			_rowMask[i] = true;
		
		// gamma precalculations
		for ( unsigned int i = 0 ; i < 2048 ; i++) {
			float v = i/2048.0;
			v = std::pow(v, 3)* 6;
			_mGamma[i] = v*6*256;
		}
	};

	~DepthReceiver() {
	};

	void receive(Message* msg) {

		//handle only RTP messages
		if (msg->getMeta("um.type") != "RTP")
			return;
		
		RScopeLock lock(_internalDepthMutex);
		bool marker = strTo<bool>(msg->getMeta("um.marker"));
		struct RTPDepthData* data = (struct RTPDepthData*)msg->data();
		
		//calculate time offset to remote host
		if (!_timeOffset)
			_timeOffset = Thread::getTimeStampMs() - data->timestamp;

		//marker is set --> new frame starts here, process old frame
		if (marker) {

#if 0
			// copy unreceived rows form neighbors
			for (unsigned int i = 0; i<480; i++) {
				if (!_rowMask[i]) {
					// find nearest neighbor row with data
					uint16_t above = i;
					uint16_t below = i;
					while(above > 0 && below < 480) {
						if (above > 0)
							above--;
						if (below < 480)
							below++;
						if (_rowMask[above]) {
							memcpy(<#void *#>, <#const void *#>, <#size_t#>)
							break;
						}
						if (_rowMask[below]) {
							break;
						}
					}
				}
			}
#endif
			
			// and clear packetloss mask
			for (unsigned int i = 0; i<480; i++)
				_rowMask[i] = false;

			
			_rtpTimestamp = strTo<uint32_t>(msg->getMeta("um.timestamp"));
			//we received a mark, so old frame is complete --> signal this to opengl thread
			if (_start) {
				_newCompleteFrame = true;
				UMUNDO_SIGNAL(_newFrame);
			}
			_start = true;
		}

		//calculate aux data
		_lastRemoteTimestamp = data->timestamp;
		_lastLocalTimestamp = Thread::getTimeStampMs() - _timeOffset;
		if (!_start)				//wait for first complete frame (and ignore data till then)
			return;
		
		_rowMask[data->row] = true;		//mark this row as received

		//process received data and calculate received depth picture row
		for (unsigned int col = 0 ; col < 640 ; col++) {
			unsigned int i = data->row * 640 + col;
			
			uint16_t pval = _mGamma[data->data[col]];
			uint8_t lb = pval & 0xff;
			
			switch (pval>>8) {
			case 0:
				_depthBuffer[3*i+0] = 255;
				_depthBuffer[3*i+1] = 255-lb;
				_depthBuffer[3*i+2] = 255-lb;
				break;
			case 1:
				_depthBuffer[3*i+0] = 255;
				_depthBuffer[3*i+1] = lb;
				_depthBuffer[3*i+2] = 0;
				break;
			case 2:
				_depthBuffer[3*i+0] = 255-lb;
				_depthBuffer[3*i+1] = 255;
				_depthBuffer[3*i+2] = 0;
				break;
			case 3:
				_depthBuffer[3*i+0] = 0;
				_depthBuffer[3*i+1] = 255;
				_depthBuffer[3*i+2] = lb;
				break;
			case 4:
				_depthBuffer[3*i+0] = 0;
				_depthBuffer[3*i+1] = 255-lb;
				_depthBuffer[3*i+2] = 255;
				break;
			case 5:
				_depthBuffer[3*i+0] = 0;
				_depthBuffer[3*i+1] = 0;
				_depthBuffer[3*i+2] = 255-lb;
				break;
			default:
				_depthBuffer[3*i+0] = 0;
				_depthBuffer[3*i+1] = 0;
				_depthBuffer[3*i+2] = 0;
				break;
			}
		}
	};

	bool getDepth(std::vector<uint8_t> &buffer) {
		RScopeLock lock(_internalDepthMutex);
		_newFrame.wait(_internalDepthMutex, 100);
		if (!_newCompleteFrame)
			return false;
		buffer.swap(_depthBuffer);

		_newCompleteFrame = false;
		return true;
	};

private:
	std::vector<uint16_t> _mGamma;
	bool _newCompleteFrame;
	uint32_t _rtpTimestamp;
	bool _start;
	uint64_t _lastRemoteTimestamp;
	uint64_t _lastLocalTimestamp;
	int64_t _timeOffset;
	bool _rowMask[480];

	std::vector<uint8_t> _depthBuffer;
	
	RMutex _internalDepthMutex;
	Monitor _newFrame;
};

DepthReceiver depthRecv;
VideoReceiver videoRecv;

void DrawGLScene() {
	static std::vector<uint8_t> frame(640*480*4);

//	depthRecv.getDepth(frame);
	videoRecv.getRGB(frame);
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, &frame[0]);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
	glTexCoord2f(0, 0);
	glVertex3f(0,0,0);
	glTexCoord2f(1, 0);
	glVertex3f(640,0,0);
	glTexCoord2f(1, 1);
	glVertex3f(640,480,0);
	glTexCoord2f(0, 1);
	glVertex3f(0,480,0);
	glEnd();

	glutSwapBuffers();
}

void ReSizeGLScene(int Width, int Height) {
	glViewport(0,0,Width,Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho (0, 640, 480, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
}

void InitGL(int Width, int Height) {
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0);
	glDepthFunc(GL_LESS);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);
	glGenTextures(1, &gl_depth_tex);
	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	ReSizeGLScene(Width, Height);
}

int main(int argc, char** argv) {
	printf("umundo-kinect-sub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	Subscriber subDepth;
	Subscriber subVideo;
	
	{
		SubscriberConfigRTP subConfig("kinect.depth");
		//subConfig.setMulticastIP("224.1.2.3");
		subDepth = Subscriber(&subConfig);
		subDepth.setReceiver(&depthRecv);
	}

	{
		SubscriberConfigRTP subConfig("kinect.video");
		//subConfig.setMulticastIP("224.1.2.3");
		subVideo = Subscriber(&subConfig);
		subVideo.setReceiver(&videoRecv);
	}

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(subDepth);
	node.addSubscriber(subVideo);


	//OpenGL Display Part
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(640, 480);
	glutInitWindowPosition(0, 0);

	glutCreateWindow("UMundo Kinect Stream");

	glutDisplayFunc(&DrawGLScene);
	glutIdleFunc(&DrawGLScene);
	glutReshapeFunc(&ReSizeGLScene);

	InitGL(640, 480);

	glutMainLoop();

	return 0;
}