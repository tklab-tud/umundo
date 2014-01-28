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

using namespace umundo;

GLuint gl_depth_tex;

class TestReceiver : public Receiver {
public:
	TestReceiver() : _baseSeq(0), _rtpTimestamp(0), _start(false), _offset(0) {
		m_buffer_depth=new uint8_t[640*480*3];
		for(unsigned int i=0; i<480; i++)
			_mask[i]=true;
	};
	~TestReceiver() {
		delete m_buffer_depth;
	};
	void receive(Message* msg) {
		if(msg->getMeta("type")=="RTP") {
			ScopeLock lock(m_depth_mutex);
			bool marker=strTo<bool>(msg->getMeta("marker"));
			uint32_t seq=strTo<uint32_t>(msg->getMeta("extendedSequenceNumber"));
			const char *raw_data=msg->data();
			uint16_t *depth=(uint16_t*)((char*)raw_data+sizeof(uint64_t));
			uint64_t timestamp=*((uint64_t*)raw_data);
			uint16_t row = (seq-_baseSeq)%480;
			
			if(!_offset)
				_offset=Thread::getTimeStampMs()-timestamp;
				
			/*
			std::cout << "RTP(" << msg->size() << ") seq: " << msg->getMeta("sequenceNumber") << " eSeq: " << msg->getMeta("extendedSequenceNumber") << " stamp: " << msg->getMeta("timestamp") << " marker: " << msg->getMeta("marker") << std::flush;
			if(marker)
				std::cout << " [advertising new frame] " << std::flush;
			std::cout << " row: " << row << std::flush;
			if(strTo<uint32_t>(msg->getMeta("timestamp"))!=_rtpTimestamp)
				std::cout << " {OUT OF ORDER TIMESTAMP " << strTo<uint32_t>(msg->getMeta("timestamp"))-_rtpTimestamp << "} " << std::flush;
			std::cout << std::endl << std::flush;
			*/
			if(marker) {
				if(_start) {
					std::cout << std::endl << "rtp timestamp " << _rtpTimestamp
						<< " (transmission delay: " << (_lastLocalTimestamp-_lastRemoteTimestamp)
						<< "ms, frames per second: " << (1000/((Thread::getTimeStampMs()-_offset)-_lastRemoteTimestamp)) << ") missed rows:";
					int start_miss=0;
					int sum=0;
					bool old=false;
					for(unsigned int i=0; i<480; i++)
					{
						if(_mask[i]!=old) {
							if(_mask[i]==true && i>0) {
								if(start_miss==i-1)
									std::cout << " " << start_miss;
								else
									std::cout << " " << start_miss << "-" << i-1;
								sum+=i-start_miss;
							}
							if(_mask[i]==false)
								start_miss=i;
						}
						old=_mask[i];
					}
					if(old==false) {
						if(start_miss==479)
							std::cout << " " << start_miss;
						else
							std::cout << " " << start_miss << "-479";
						sum+=479-start_miss+1;
					}
					std::cout << " sum: " << sum << " ("<< ((double)sum/480.0)*100.0 << "%)" << std::endl;
				}
				for(unsigned int i=0; i<480; i++)
					_mask[i]=false;
				_baseSeq = seq;
				_start=true;
				_rtpTimestamp=strTo<uint32_t>(msg->getMeta("timestamp"));
				m_new_depth_frame = true;
				UMUNDO_SIGNAL(_newFrame);
			}
			_lastRemoteTimestamp=timestamp;
			_lastLocalTimestamp=Thread::getTimeStampMs()-_offset;
			if(!_start)		//wait for first complete frame
				return;
			_mask[row]=true;
			for( unsigned int i = row*640 ; i < row*640 + msg->size()/sizeof(uint16_t) ; i++) {
				uint16_t pval = depth[i-row*640];
				uint8_t lb = pval & 0xff;
				switch (pval>>8) {
				case 0:
					m_buffer_depth[3*i+0] = 255;
					m_buffer_depth[3*i+1] = 255-lb;
					m_buffer_depth[3*i+2] = 255-lb;
					break;
				case 1:
					m_buffer_depth[3*i+0] = 255;
					m_buffer_depth[3*i+1] = lb;
					m_buffer_depth[3*i+2] = 0;
					break;
				case 2:
					m_buffer_depth[3*i+0] = 255-lb;
					m_buffer_depth[3*i+1] = 255;
					m_buffer_depth[3*i+2] = 0;
					break;
				case 3:
					m_buffer_depth[3*i+0] = 0;
					m_buffer_depth[3*i+1] = 255;
					m_buffer_depth[3*i+2] = lb;
					break;
				case 4:
					m_buffer_depth[3*i+0] = 0;
					m_buffer_depth[3*i+1] = 255-lb;
					m_buffer_depth[3*i+2] = 255;
					break;
				case 5:
					m_buffer_depth[3*i+0] = 0;
					m_buffer_depth[3*i+1] = 0;
					m_buffer_depth[3*i+2] = 255-lb;
					break;
				default:
					m_buffer_depth[3*i+0] = 0;
					m_buffer_depth[3*i+1] = 0;
					m_buffer_depth[3*i+2] = 0;
					break;
				}
			}
		}
	}
	bool getDepth(uint8_t *buffer) {
		ScopeLock lock(m_depth_mutex);
		_newFrame.wait(m_depth_mutex, 100);
		if (!m_new_depth_frame)
			return false;
		memcpy(buffer, m_buffer_depth, 640*480*3*sizeof(uint8_t));
		m_new_depth_frame = false;
		return true;
	}
private:
	bool m_new_depth_frame;
	uint8_t *m_buffer_depth;
	uint32_t _baseSeq;
	uint32_t _rtpTimestamp;
	bool _start;
	uint64_t _lastRemoteTimestamp;
	uint64_t _lastLocalTimestamp;
	int64_t _offset;
	bool _mask[480];
	Mutex m_depth_mutex;
	Monitor _newFrame;
};

TestReceiver testRecv;

void DrawGLScene()
{
	uint8_t depth[640*480*3];

	testRecv.getDepth(depth);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, gl_depth_tex);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, depth);

	glBegin(GL_TRIANGLE_FAN);
	glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
	glTexCoord2f(0, 0); glVertex3f(0,0,0);
	glTexCoord2f(1, 0); glVertex3f(640,0,0);
	glTexCoord2f(1, 1); glVertex3f(640,480,0);
	glTexCoord2f(0, 1); glVertex3f(0,480,0);
	glEnd();

	glutSwapBuffers();
}

void ReSizeGLScene(int Width, int Height)
{
	glViewport(0,0,Width,Height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho (0, 640, 480, 0, -1.0f, 1.0f);
	glMatrixMode(GL_MODELVIEW);
}

void InitGL(int Width, int Height)
{
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

	RTPSubscriberConfig subConfig;
	//subConfig.setMulticastIP("224.1.2.3");
	//subConfig.setMulticastPortbase(42142);
	Subscriber subFoo(Subscriber::RTP, "kinect-pubsub", &testRecv, &subConfig);

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
