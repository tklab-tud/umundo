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

#include <portaudio.h>

#define SAMPLE_RATE (16000)
#define FRAMES_PER_BUFFER (64)

using namespace umundo;

PaStream *output_stream;

void checkError(PaError err, int fatal=1) {
	if( err == paNoError )
		return;
	if(fatal)
		Pa_Terminate();
//	std::cout << "An error occured while using the portaudio stream" << std::endl;
//	std::cout << "Error number: " << err << std::endl;
//	std::cout << "Error message: " << Pa_GetErrorText( err ) << std::endl;
	if(fatal)
		exit(err);
}

class TestReceiver : public Receiver {
public:
	TestReceiver() {};
	void receive(Message* msg) {
		if(msg->getMeta("um.type")=="RTP") {
//			std::cout << "RTP(" << msg->size() << ")" << std::endl << std::flush;
			if(Pa_IsStreamStopped(output_stream)==1) {				//start output stream when first packed is received
//				Thread::sleepMs(10);									//wait some time to compensate network delay
				checkError(Pa_StartStream(output_stream));
			}
			checkError(Pa_WriteStream(output_stream, msg->data(), FRAMES_PER_BUFFER), 0);
		}
	}
};

int main(int argc, char** argv) {
	float buffer[FRAMES_PER_BUFFER];
	PaStreamParameters inputParameters;
	PaStreamParameters outputParameters;
	PaStream *input_stream;

	printf("umundo-phone version " UMUNDO_VERSION " (" CMAKE_BUILD_TYPE " build)\n");

	// data with sample rate of 16000Hz and 4ms payload per rtp packet (64 samples)
	RTPPublisherConfig pubConfig(FRAMES_PER_BUFFER);
	Publisher pubFoo(Publisher::RTP, "phone", &pubConfig);

	TestReceiver testRecv;
	RTPSubscriberConfig subConfig;
	Subscriber subFoo(Subscriber::RTP, "phone", &testRecv, &subConfig);

	Discovery disc(Discovery::MDNS);
	Node node;
	disc.add(node);
	node.addPublisher(pubFoo);
	node.addSubscriber(subFoo);

	checkError(Pa_Initialize());

	inputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (inputParameters.device == paNoDevice) {
		std::cout << "Error: No default output device." << std::endl;
		return 0;
	}
	inputParameters.device = Pa_GetDefaultInputDevice(); /* default input device */
	inputParameters.channelCount = 1;
	inputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	inputParameters.suggestedLatency = Pa_GetDeviceInfo( inputParameters.device )->defaultHighInputLatency ;
	inputParameters.hostApiSpecificStreamInfo = NULL;

	outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
	if (outputParameters.device == paNoDevice) {
		std::cout << "Error: No default output device." << std::endl;
		return 0;
	}
	outputParameters.channelCount = 1; /* mono output */
	outputParameters.sampleFormat = paFloat32; /* 32 bit floating point output */
	outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultHighOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = NULL;

	checkError(Pa_OpenStream(
	               &input_stream,
	               &inputParameters,
	               NULL, /* no output */
	               SAMPLE_RATE,
	               FRAMES_PER_BUFFER,
	               paClipOff, /* we won't output out of range samples so don't bother clipping them */
	               NULL, /* no callback, use blocking API */
	               NULL )); /* no callback, so no callback userData */

	checkError(Pa_OpenStream(
	               &output_stream,
	               NULL, /* no input */
	               &outputParameters,
	               SAMPLE_RATE,
	               FRAMES_PER_BUFFER,
	               paClipOff, /* we won't output out of range samples so don't bother clipping them */
	               NULL, /* no callback, use blocking API */
	               NULL )); /* no callback, so no callback userData */

	checkError(Pa_StartStream(input_stream));
	while(1) {
		checkError(Pa_ReadStream(input_stream, buffer, FRAMES_PER_BUFFER), 0);

		Message* msg = new Message();
		msg->setData((char*)buffer, sizeof(float)*FRAMES_PER_BUFFER);
		pubFoo.send(msg);
		delete(msg);
	}

	return 0;
}
