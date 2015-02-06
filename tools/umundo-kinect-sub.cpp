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

#ifdef WIN32
#include "XGetopt.h"
#endif

#define MAX_PAYLOAD_PACKET 800
#define VIDEO_BUFFER_SIZE 640 * 480 * 3

bool useDepth = false;
bool useVideo = false;
bool useRTP   = false;
bool useMCast = false;
bool useTCP   = false;

using namespace umundo;

GLuint gl_depth_tex;

void bayer_to_rgb(uint8_t *raw_buf, uint8_t *proc_buf, size_t width, size_t height);

class VideoReceiver : public Receiver {
public:
	VideoReceiver() : _frameComplete(false), _start(false), _videoBuffer(VIDEO_BUFFER_SIZE), _rtpBuffer(VIDEO_BUFFER_SIZE) {
	}

	~VideoReceiver() {
	}

	void receive(Message* msg) {
		RScopeLock lock(_internalVideoMutex);
		if (msg->getMeta("um.type") != "RTP") {
			// received via TCP
			bayer_to_rgb((uint8_t*)msg->data(), (uint8_t*)&_videoBuffer[0], 320, 240);
			_frameComplete = true;
			UMUNDO_SIGNAL(_newFrame);

		} else {
			// received via RTP
			bool marker = strTo<bool>(msg->getMeta("um.marker"));

			if (marker) {
				if (_start) {
					bayer_to_rgb((uint8_t*)&_rtpBuffer[0], (uint8_t*)&_videoBuffer[0], 320, 240);
//					memset(&_rtpBuffer[0], 255, _rtpBuffer.size());
					_frameComplete = true;
					UMUNDO_SIGNAL(_newFrame);
				}
				_start = true;
			}

			uint16_t index;
			Message::read(msg->data(), &index);

			if (!_start)				//wait for first complete frame (and ignore data till then)
				return;

			memcpy(&_rtpBuffer[index * MAX_PAYLOAD_PACKET], msg->data() + 2, msg->size() - 2);
		}
	}

	bool getRGB(std::vector<uint8_t> &buffer) {
		RScopeLock lock(_internalVideoMutex);
		_newFrame.wait(_internalVideoMutex, 100);

		if (!_frameComplete)
			return false;

		buffer.swap(_videoBuffer);
		_frameComplete = false;

		return true;
	}

protected:
	bool _frameComplete;
	bool _start;

	RMutex _internalVideoMutex;
	Monitor _newFrame;

	std::vector<uint8_t> _videoBuffer;
	std::vector<uint8_t> _rtpBuffer;

};

class DepthReceiver : public Receiver {
public:
	DepthReceiver() : _mGamma(2048), _rtpTimestamp(0), _start(false), _timeOffset(0), _depthBuffer(640*480*3) {
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
		RScopeLock lock(_internalDepthMutex);
		bool marker = strTo<bool>(msg->getMeta("um.marker"));

		//marker is set --> new frame starts here, process old frame
		if (marker) {
			if (_start) {
				_newCompleteFrame = true;
				UMUNDO_SIGNAL(_newFrame);
			}
			_start = true;
		}

		if (!_start)				//wait for first complete frame (and ignore data till then)
			return;

		uint16_t index;
		Message::read(msg->data(), &index);

#if 0
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
#endif
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

DepthReceiver* depthRecv;
VideoReceiver* videoRecv;

void DrawGLScene() {
	static std::vector<uint8_t> frame(640*480*3);

	if (useVideo) {
		videoRecv->getRGB(frame);
	} else {
		depthRecv->getDepth(frame);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 320, 240, 0, GL_RGB, GL_UNSIGNED_BYTE, &frame[0]);

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

void printUsageAndExit() {
	printf("umundo-kinect-sub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");
	printf("Usage\n");
	printf("\tumundo-kinect-sub -t[rtp|tcp|mcast] -c[depth|video]\n");
	printf("\n");
	printf("Options\n");
	printf("\t-t [rtp|tcp|mcast] : type of publisher to connect to\n");
	printf("\t-c [depth|video]   : camera to use\n");
	exit(1);
}

int main(int argc, char** argv) {

	int option;
	while ((option = getopt(argc, argv, "t:c:")) != -1) {
		switch(option) {
		case 't':
			if (strncasecmp(optarg, "tcp", 3) == 0) {
				useTCP = true;
			} else if (strncasecmp(optarg, "mcast", 5) == 0) {
				useMCast = true;
			} else if (strncasecmp(optarg, "rtp", 3) == 0) {
				useRTP = true;
			} else {
				printUsageAndExit();
			}
			break;
		case 'c':
			if (strncasecmp(optarg, "depth", 5) == 0) {
				useDepth = true;
				std::cerr << "Sorry, I broke depth reception for now" << std::endl;
				printUsageAndExit();
			} else if (strncasecmp(optarg, "video", 5) == 0) {
				useVideo = true;
			} else {
				printUsageAndExit();
			}
			break;
		default:
			printUsageAndExit();
			break;
		}
	}

	Subscriber sub;
	std::string channelName;
	Receiver* recv;

	if (!useDepth && !useVideo)
		useVideo = true;

	if (!useTCP && !useRTP)
		useRTP = true;

	if (useVideo) {
		channelName = "kinect.video";
		recv = videoRecv = new VideoReceiver();
	} else {
		channelName = "kinect.depth";
		recv = depthRecv = new DepthReceiver();
	}

	if (useTCP) {
		SubscriberConfigTCP subConfig(channelName + ".tcp");
		sub = Subscriber(&subConfig);
		sub.setReceiver(recv);
	} else {
		SubscriberConfigRTP subConfig(channelName + ".rtp");
		if (useMCast)
			subConfig.setMulticastIP("224.1.2.3");

		sub = Subscriber(&subConfig);
		sub.setReceiver(recv);
	}

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(sub);


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

// from libfreenect cameras.c
void bayer_to_rgb(uint8_t *raw_buf, uint8_t *proc_buf, size_t width, size_t height) {
	int x,y;

	uint8_t *dst = proc_buf; // pointer to destination

	uint8_t *prevLine;        // pointer to previous, current and next line
	uint8_t *curLine;         // of the source bayer pattern
	uint8_t *nextLine;

	// storing horizontal values in hVals:
	// previous << 16, current << 8, next
	uint32_t hVals;
	// storing vertical averages in vSums:
	// previous << 16, current << 8, next
	uint32_t vSums;

	// init curLine and nextLine pointers
	curLine  = raw_buf;
	nextLine = curLine + width;
	for (y = 0; y < height; ++y) {

		if ((y > 0) && (y < height-1))
			prevLine = curLine - width; // normal case
		else if (y == 0)
			prevLine = nextLine;      // top boundary case
		else
			nextLine = prevLine;      // bottom boundary case

		// init horizontal shift-buffer with current value
		hVals  = (*(curLine++) << 8);
		// handle left column boundary case
		hVals |= (*curLine << 16);
		// init vertical average shift-buffer with current values average
		vSums = ((*(prevLine++) + *(nextLine++)) << 7) & 0xFF00;
		// handle left column boundary case
		vSums |= ((*prevLine + *nextLine) << 15) & 0xFF0000;

		// store if line is odd or not
		uint8_t yOdd = y & 1;
		// the right column boundary case is not handled inside this loop
		// thus the "639"
		for (x = 0; x < width-1; ++x) {
			// place next value in shift buffers
			hVals |= *(curLine++);
			vSums |= (*(prevLine++) + *(nextLine++)) >> 1;

			// calculate the horizontal sum as this sum is needed in
			// any configuration
			uint8_t hSum = ((uint8_t)(hVals >> 16) + (uint8_t)(hVals)) >> 1;

			if (yOdd == 0) {
				if ((x & 1) == 0) {
					// Configuration 1
					*(dst++) = hSum;		// r
					*(dst++) = hVals >> 8;	// g
					*(dst++) = vSums >> 8;	// b
				} else {
					// Configuration 2
					*(dst++) = hVals >> 8;
					*(dst++) = (hSum + (uint8_t)(vSums >> 8)) >> 1;
					*(dst++) = ((uint8_t)(vSums >> 16) + (uint8_t)(vSums)) >> 1;
				}
			} else {
				if ((x & 1) == 0) {
					// Configuration 3
					*(dst++) = ((uint8_t)(vSums >> 16) + (uint8_t)(vSums)) >> 1;
					*(dst++) = (hSum + (uint8_t)(vSums >> 8)) >> 1;
					*(dst++) = hVals >> 8;
				} else {
					// Configuration 4
					*(dst++) = vSums >> 8;
					*(dst++) = hVals >> 8;
					*(dst++) = hSum;
				}
			}

			// shift the shift-buffers
			hVals <<= 8;
			vSums <<= 8;
		} // end of for x loop
		// right column boundary case, mirroring second last column
		hVals |= (uint8_t)(hVals >> 16);
		vSums |= (uint8_t)(vSums >> 16);

		// the horizontal sum simplifies to the second last column value
		uint8_t hSum = (uint8_t)(hVals);

		if (yOdd == 0) {
			if ((x & 1) == 0) {
				*(dst++) = hSum;
				*(dst++) = hVals >> 8;
				*(dst++) = vSums >> 8;
			} else {
				*(dst++) = hVals >> 8;
				*(dst++) = (hSum + (uint8_t)(vSums >> 8)) >> 1;
				*(dst++) = vSums;
			}
		} else {
			if ((x & 1) == 0) {
				*(dst++) = vSums;
				*(dst++) = (hSum + (uint8_t)(vSums >> 8)) >> 1;
				*(dst++) = hVals >> 8;
			} else {
				*(dst++) = vSums >> 8;
				*(dst++) = hVals >> 8;
				*(dst++) = hSum;
			}
		}

	} // end of for y loop

}
