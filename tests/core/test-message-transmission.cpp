#include "umundo/core.h"
#include "umundo/util.h"
#include "umundo/config.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024

using namespace umundo;

static int nrReceptions = 0;
static int bytesRecvd = 0;
static int nrMissing = 0;
static std::string hostId;

class TestReceiver : public Receiver {
	void receive(Message* msg) {
		if (msg->size() > 0 && msg->getMeta("md5").length() > 0)
			assert(msg->getMeta("md5").compare(md5(msg->data(), msg->size())) == 0);
		if (nrReceptions + nrMissing == strTo<int>(msg->getMeta("seq"))) {
//			std::cout << ".";
		} else {
			std::cout << "R" << strTo<int>(msg->getMeta("seq"));
		}
		while (nrReceptions + nrMissing < strTo<int>(msg->getMeta("seq"))) {
			nrMissing++;
			std::cout << "F" << nrReceptions + nrMissing;
		}
		nrReceptions++;
		bytesRecvd += msg->size();
	}
};

bool testMessageTransmission() {
	hostId = Host::getHostId();
	for (int i = 0; i < 2; i++) {
		nrReceptions = 0;
		nrMissing = 0;
		bytesRecvd = 0;

		Node pubNode;
		Publisher pub("foo");
		pubNode.addPublisher(pub);

		TestReceiver* testRecv = new TestReceiver();
		Node subNode;
		Subscriber sub("foo");
		sub.setReceiver(testRecv);
		subNode.addSubscriber(sub);

		subNode.add(pubNode);
		pubNode.add(subNode);

		pub.waitForSubscribers(1);
		assert(pub.waitForSubscribers(0) == 1);

		int iterations = 1000;

		for (int j = 0; j < iterations; j++) {
			Message* msg = new Message();
			msg->putMeta("seq", toStr(j));
			pub.send(msg);
			delete msg;
		}

		// wait until all messages are delivered
		Thread::sleepMs(500);

		// sometimes there is some weird latency
		for (int i = 0; i < 5; i++) {
			if (nrReceptions < iterations)
				Thread::sleepMs(2000);
		}

		std::cout << "expected " << iterations << " messages, received " << nrReceptions << std::endl;
		assert(nrReceptions == iterations);

		subNode.removeSubscriber(sub);
		pubNode.removePublisher(pub);
	}
	return true;
}

bool testDataTransmission() {
	hostId = Host::getHostId();
	for (int i = 0; i < 2; i++) {
		nrReceptions = 0;
		nrMissing = 0;
		bytesRecvd = 0;

		Node pubNode;
		Publisher pub("foo");
		pubNode.addPublisher(pub);

		TestReceiver* testRecv = new TestReceiver();
		Node subNode;
		Subscriber sub("foo");
		sub.setReceiver(testRecv);
		subNode.addSubscriber(sub);

		subNode.add(pubNode);
		pubNode.add(subNode);

		pub.waitForSubscribers(1);
		assert(pub.waitForSubscribers(0) == 1);

		char* buffer = (char*)malloc(BUFFER_SIZE);
		memset(buffer, 40, BUFFER_SIZE);

		int iterations = 1000;

		for (int j = 0; j < iterations; j++) {
			Message* msg = new Message(buffer, BUFFER_SIZE);
//			msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
			msg->putMeta("seq",toStr(j));
			pub.send(msg);
			delete msg;
		}

		// wait until all messages are delivered
		Thread::sleepMs(500);

		// sometimes there is some weird latency
		for (int i = 0; i < 20; i++) {
			if (nrReceptions < iterations)
				Thread::sleepMs(500);
		}

		std::cout << "expected " << nrReceptions * BUFFER_SIZE << " bytes, received " << bytesRecvd << std::endl;
		assert(nrReceptions == iterations);
		assert(bytesRecvd == nrReceptions * BUFFER_SIZE);

		subNode.removeSubscriber(sub);
		pubNode.removePublisher(pub);

	}
	return true;
}

bool testByteWriting() {
	char* data = (char*)malloc(1000);

	uint64_t value = 1;

	for (int shift = 0; shift < 64; shift++, value <<= 1) {
		char* writePtr = data;
		const char* readPtr = data;

		uint64_t uint64 = value;
		uint32_t uint32 = value;
		uint16_t uint16 = value;
		uint8_t uint8 = value;

		int64_t int64 = value;
		int32_t int32 = value;
		int16_t int16 = value;
		int8_t int8 = value;

		float fl = (float)value;
		double dbl = (double)value;

		writePtr = Message::write(writePtr, uint64);
		readPtr = Message::read(readPtr, &uint64);

		writePtr = Message::write(writePtr, uint32);
		readPtr = Message::read(readPtr, &uint32);

		writePtr = Message::write(writePtr, uint16);
		readPtr = Message::read(readPtr, &uint16);

		writePtr = Message::write(writePtr, uint8);
		readPtr = Message::read(readPtr, &uint8);

		writePtr = Message::write(writePtr, int64);
		readPtr = Message::read(readPtr, &int64);

		writePtr = Message::write(writePtr, int32);
		readPtr = Message::read(readPtr, &int32);

		writePtr = Message::write(writePtr, int16);
		readPtr = Message::read(readPtr, &int16);

		writePtr = Message::write(writePtr, int8);
		readPtr = Message::read(readPtr, &int8);

		writePtr = Message::write(writePtr, fl);
		readPtr = Message::read(readPtr, &fl);

		writePtr = Message::write(writePtr, dbl);
		readPtr = Message::read(readPtr, &dbl);

		assert(dbl = (double)value);
		assert(uint64 = (uint64_t)value);
		assert(int64 = (int64_t)value);

		if (shift >= 32)
			continue;

		assert(fl = (float)value);
		assert(uint32 = (uint32_t)value);
		assert(int32 = (int32_t)value);

		if (shift >= 16)
			continue;

		assert(uint16 = (uint16_t)value);
		assert(int16 = (int16_t)value);

		if (shift >= 8)
			continue;

		assert(uint8 = (uint8_t)value);
		assert(int8 = (int8_t)value);
	}

	{
		char* writePtr = NULL;
		const char* readPtr = NULL;

		std::string readFrom;
		std::string writeTo;

		// standard case
		writePtr = data;
		readPtr = data;
		readFrom = "This is foo";
		writePtr = Message::write(writePtr, readFrom);
		assert(writePtr == data + readFrom.size() + 1);

		readPtr = Message::read(readPtr, writeTo, 100);
		assert(readPtr == data + readFrom.size() + 1);
		assert(readFrom == writeTo);

		// no termination
		writePtr = data;
		readPtr = data;
		readFrom = "This is foo";
		writePtr = Message::write(writePtr, readFrom, false);
		assert(writePtr == data + readFrom.size());
		writePtr = Message::write(writePtr, readFrom, false);
		assert(writePtr == data + 2 * readFrom.size());

		readPtr = Message::read(readPtr, writeTo, 100);
		assert(readPtr == data + 2 * readFrom.size() + 1);
		assert(writeTo == "This is fooThis is foo");

		// read only fragment back
		writePtr = data;
		readPtr = data;
		readFrom = "This is foo";

		writePtr = Message::write(writePtr, readFrom, true);
		assert(writePtr == data + readFrom.size() + 1);

		readPtr = Message::read(readPtr, writeTo, 6);
		assert(readPtr == data + 6); // we did not consume any terminator
		assert(writeTo == "This i");

		readPtr = Message::read(readPtr, writeTo, 5);
		assert(readPtr == data + 11); // we did not consume any terminator
		assert(writeTo == "s foo");

		// there is a single \0 byte remaining
		readPtr = Message::read(readPtr, writeTo, 500);
		assert(readPtr == data + 12); // we *did* not consume the terminator
		assert(writeTo == "");

	}

	return true;
}

bool testCompression() {
	std::string test1;
	for (int i = 0; i < 20; i++)
		test1 += "This is some test right here!";

	Message msg(test1.data(), test1.size());
	assert(test1 == std::string(msg.data(), msg.size()));

	msg.uncompress();

	msg.compress();
	msg.compress();
	msg.compress();
	std::cout << test1.size() << " vs " << msg.size() << std::endl;
	msg.uncompress();
	msg.uncompress();

	assert(test1 == std::string(msg.data(), msg.size()));

	// test a compressing publisher
	Node pubNode;
	PublisherConfigTCP pubConfig("bar");
#ifdef BUILD_WITH_COMPRESSION_LEVEL_LZ4
    pubConfig.enableCompression(Message::COMPRESS_LZ4);
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_MINIZ)
    pubConfig.enableCompression(Message::COMPRESS_MINIZ);
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_FASTLZ)
    pubConfig.enableCompression(Message::COMPRESS_FASTLZ);
#endif
	Publisher pub(&pubConfig);
	pubNode.addPublisher(pub);

	TestReceiver* testRecv = new TestReceiver();
	Node subNode;
	Subscriber sub("bar");
	sub.setReceiver(testRecv);
	subNode.addSubscriber(sub);

	subNode.add(pubNode);
	pubNode.add(subNode);

	pub.waitForSubscribers(1);
	assert(pub.waitForSubscribers(0) == 1);

	int iterations = 1000;

	for (int j = 0; j < iterations; j++) {
		char* buffer = (char*)malloc(BUFFER_SIZE);
		memset(buffer, j, BUFFER_SIZE);

		Message* msg = new Message(buffer, BUFFER_SIZE, Message::ADOPT_DATA);
		msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
		msg->putMeta("seq",toStr(j));
		pub.send(msg);
		delete msg;
	}
	Thread::sleepMs(500);

	return true;
}

int main(int argc, char** argv, char** envp) {
	if (!testByteWriting())
		return EXIT_FAILURE;
	if (!testCompression())
		return EXIT_FAILURE;
	if (!testMessageTransmission())
		return EXIT_FAILURE;
	if (!testDataTransmission())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
