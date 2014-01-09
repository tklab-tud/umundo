/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#include <math.h>
#include <portaudio.h>

#define SAMPLE_RATE (8000)
#define FRAMES_PER_BUFFER (332)

using namespace umundo;

PaStream *stream;

void checkError(PaError err, int fatal=1)
{
	if( err == paNoError )
		return;
	if(fatal)
		Pa_Terminate();
	std::cout << "An error occured while using the portaudio stream" << std::endl;
	std::cout << "Error number: " << err << std::endl;
	std::cout << "Error message: " << Pa_GetErrorText( err ) << std::endl;
	if(fatal)
		exit(err);
}

class TestReceiver : public Receiver {
public:
	TestReceiver() {};
	void receive(Message* msg) {
		if(msg->getMeta("type")=="RTP")
		{
			std::cout << "RTP(" << msg->size() << ")" << std::endl << std::flush;
			if(Pa_IsStreamStopped(stream)==1)				//start output stream when first packed is received
			{
				Thread::sleepMs(200);						//wait some time to compensate network delay
				checkError(Pa_StartStream(stream));
			}
			checkError(Pa_WriteStream(stream, msg->data(), msg->size()/sizeof(float)), 0);		//32 bit floating point output
		}
	}
};

int main(int argc, char** argv) {
	PaStreamParameters outputParameters;
	
	printf("umundo-phone-sub version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	TestReceiver testRecv;
	RTPSubscriberConfig subConfig;
	Subscriber subFoo(Subscriber::RTP, "phone-pubsub", &testRecv, &subConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addSubscriber(subFoo);

	checkError(Pa_Initialize());

	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (outputParameters.device == paNoDevice) {
		std::cout << "Error: No default output device." << std::endl;
		return 0;
	}
	outputParameters.channelCount = 1; /* mono output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;
	
	checkError(Pa_OpenStream(
		&stream,
		NULL, /* no input */
		&outputParameters,
		SAMPLE_RATE,
		FRAMES_PER_BUFFER,
		paClipOff, /* we won't output out of range samples so don't bother clipping them */
		NULL, /* no callback, use blocking API */
		NULL )); /* no callback, so no callback userData */

	while(1)
		Thread::sleepMs(4000);

	return 0;
}
