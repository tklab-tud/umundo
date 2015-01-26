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

struct RTPData {
	uint16_t row;
	uint64_t timestamp;
	uint16_t data[640];
};

using namespace umundo;

GLuint gl_depth_tex;

class TestReceiver : public Receiver {
public:
	TestReceiver() : _mGamma(2048), _rtpTimestamp(0), _start(false), _timeOffset(0) {
		_internalDepthBuffer=new uint8_t[640*480*3];
		for(unsigned int i=0; i<480; i++)
			_mask[i]=true;
		//gamma precalculations
		for( unsigned int i = 0 ; i < 2048 ; i++) {
			float v = i/2048.0;
			v = std::pow(v, 3)* 6;
			_mGamma[i] = v*6*256;
		}
	};

	~TestReceiver() {
		delete _internalDepthBuffer;
	};

	void receive(Message* msg) {

		//handle only RTP messages
		if(msg->getMeta("um.type")=="RTP") {
			RScopeLock lock(_internalDepthMutex);
			bool marker=strTo<bool>(msg->getMeta("marker"));
			struct RTPData *data=(struct RTPData*)msg->data();

			/*
			 std::cout << "RTP(" << msg->size() << ") sequenceNumber: " << msg->getMeta("sequenceNumber") << " extendedSequenceNumber: " << msg->getMeta("extendedSequenceNumber") << " rtpTimestamp: " << msg->getMeta("timestamp") << " marker: " << msg->getMeta("marker") << std::flush;
			 if(marker)
			 std::cout << " [advertising new frame] " << std::flush;
			 std::cout << " row: " << data->row << std::flush;
			 if(strTo<uint32_t>(msg->getMeta("timestamp"))!=_rtpTimestamp)
			 std::cout << " {OUT OF ORDER TIMESTAMP " << strTo<uint32_t>(msg->getMeta("timestamp"))-_rtpTimestamp << "} " << std::flush;
			 std::cout << std::endl << std::flush;
			 */

			//calculate time offset to remote host
			if(!_timeOffset)
				_timeOffset=Thread::getTimeStampMs()-data->timestamp;

			//marker is set --> new frame starts here
			if(marker) {
				if(_start) {
					//output info for last complete frame
#if 0
					std::cout << std::endl << "rtp timestamp " << _rtpTimestamp
					          << " (remote time offset: " << _timeOffset
					          << "ms, transmission delay: " << (_lastLocalTimestamp-_lastRemoteTimestamp)
					          << "ms, frames per second: " << (1000/((Thread::getTimeStampMs()-_timeOffset)-_lastRemoteTimestamp)) << ")";
#endif
					//calculate packet misses (and output statistics about them)
					int start_miss=0;
					int sum=0;
					bool old=false;
					std::stringstream stream;
					for(unsigned int i=0; i<480; i++) {
						if(_mask[i]!=old) {
							if(_mask[i]==true && i>0) {
								if(start_miss==i-1)
									stream << " " << start_miss;
								else
									stream << " " << start_miss << "-" << i-1;
								sum+=i-start_miss;
							}
							if(_mask[i]==false)
								start_miss=i;
						}
						old=_mask[i];
					}
					if(old==false) {
						if(start_miss==479)
							stream << " " << start_miss;
						else
							stream << " " << start_miss << "-479";
						sum+=479-start_miss+1;
					}
#if 0
					std::cout << " missed rows sum: " << sum << " ("<< ((double)sum/480.0)*100.0 << "%)";
					if(sum)
						std::cout << " missed rows detail: " << stream.str();
					std::cout << std::endl << std::flush;
#endif
				}
				//clear packetloss mask
				for(unsigned int i=0; i<480; i++)
					_mask[i]=false;
				_rtpTimestamp=strTo<uint32_t>(msg->getMeta("um.timestamp"));
				//we received a mark, so old frame is complete --> signal this to opengl thread
				if(_start) {
					_newCompleteFrame = true;
					UMUNDO_SIGNAL(_newFrame);
				}
				_start=true;
			}

			//calculate aux data
			_lastRemoteTimestamp=data->timestamp;
			_lastLocalTimestamp=Thread::getTimeStampMs()-_timeOffset;
			if(!_start)				//wait for first complete frame (and ignore data till then)
				return;
			_mask[data->row]=true;		//mark this row as received

			//process received data and calculate received depth picture row
			for(unsigned int col = 0 ; col < 640 ; col++) {
				unsigned int i=data->row*640 + col;
				uint16_t pval = _mGamma[data->data[col]];
				uint8_t lb = pval & 0xff;
				switch (pval>>8) {
				case 0:
					_internalDepthBuffer[3*i+0] = 255;
					_internalDepthBuffer[3*i+1] = 255-lb;
					_internalDepthBuffer[3*i+2] = 255-lb;
					break;
				case 1:
					_internalDepthBuffer[3*i+0] = 255;
					_internalDepthBuffer[3*i+1] = lb;
					_internalDepthBuffer[3*i+2] = 0;
					break;
				case 2:
					_internalDepthBuffer[3*i+0] = 255-lb;
					_internalDepthBuffer[3*i+1] = 255;
					_internalDepthBuffer[3*i+2] = 0;
					break;
				case 3:
					_internalDepthBuffer[3*i+0] = 0;
					_internalDepthBuffer[3*i+1] = 255;
					_internalDepthBuffer[3*i+2] = lb;
					break;
				case 4:
					_internalDepthBuffer[3*i+0] = 0;
					_internalDepthBuffer[3*i+1] = 255-lb;
					_internalDepthBuffer[3*i+2] = 255;
					break;
				case 5:
					_internalDepthBuffer[3*i+0] = 0;
					_internalDepthBuffer[3*i+1] = 0;
					_internalDepthBuffer[3*i+2] = 255-lb;
					break;
				default:
					_internalDepthBuffer[3*i+0] = 0;
					_internalDepthBuffer[3*i+1] = 0;
					_internalDepthBuffer[3*i+2] = 0;
					break;
				}
			}
		}
	};

	bool getDepth(uint8_t *buffer) {
		RScopeLock lock(_internalDepthMutex);
		_newFrame.wait(_internalDepthMutex, 100);
		if (!_newCompleteFrame)
			return false;
		//copy buffer so that the frame doesn't change while displaying it with opengl (while already receiving packets for the next frame)
		memcpy(buffer, _internalDepthBuffer, 640*480*3*sizeof(uint8_t));
		_newCompleteFrame = false;
		return true;
	};

private:
	std::vector<uint16_t> _mGamma;
	bool _newCompleteFrame;
	uint8_t *_internalDepthBuffer;
	uint32_t _rtpTimestamp;
	bool _start;
	uint64_t _lastRemoteTimestamp;
	uint64_t _lastLocalTimestamp;
	int64_t _timeOffset;
	bool _mask[480];
	RMutex _internalDepthMutex;
	Monitor _newFrame;
};

TestReceiver testRecv;

void DrawGLScene() {
	uint8_t depth[640*480*3];

	testRecv.getDepth(depth);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, depth);

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

	SubscriberConfigRTP subConfig("kinect-pubsub");
	//subConfig.setMulticastIP("224.1.2.3");
	//subConfig.setMulticastPortbase(42142);
	Subscriber subFoo(&subConfig);
	subFoo.setReceiver(&testRecv);
	
	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(subFoo);


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