#include "umundo.h"
#include "umundo/config.h"
#include "umundo/util/crypto/MD5.h"
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
        assert(msg->getMeta("seq").size() > 0);
        if (msg->size() > 0 && msg->getMeta("md5").length() > 0)
            assert(msg->getMeta("md5").compare(md5(msg->data(), msg->size())) == 0);
        if (nrReceptions + nrMissing == strTo<int>(msg->getMeta("seq"))) {
            //std::cout << ".";
        } else {
            std::cout << " R" << strTo<int>(msg->getMeta("seq"));
        }
        while (nrReceptions + nrMissing < strTo<int>(msg->getMeta("seq"))) {
            nrMissing++;
            std::cout << " F" << nrReceptions + nrMissing;
        }
        nrReceptions++;
        bytesRecvd += msg->size();
    }
};

bool testByteWriting() {
    char* data = (char*)malloc(1000);
    
    uint64_t value = 1;
    
    {
        char* writePtr = data;
        const char* readPtr = data;
        uint64_t v;
        
        writePtr = Message::writeCompact(writePtr, 250, 1);
        assert(writePtr == data + 1);
        writePtr = Message::writeCompact(writePtr, 65000, 3);
        assert(writePtr == data + 4);
        writePtr = Message::writeCompact(writePtr, 1000000, 9);
        assert(writePtr == data + 13);
        
        readPtr = Message::readCompact(readPtr, &v, 1);
        assert(v == 250);
        readPtr = Message::readCompact(readPtr, &v, 3);
        assert(v == 65000);
        readPtr = Message::readCompact(readPtr, &v, 9);
        assert(v == 1000000);

    }
    
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
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, uint32);
        readPtr = Message::read(readPtr, &uint32);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, uint16);
        readPtr = Message::read(readPtr, &uint16);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, uint8);
        readPtr = Message::read(readPtr, &uint8);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, int64);
        readPtr = Message::read(readPtr, &int64);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, int32);
        readPtr = Message::read(readPtr, &int32);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, int16);
        readPtr = Message::read(readPtr, &int16);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, int8);
        readPtr = Message::read(readPtr, &int8);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, fl);
        readPtr = Message::read(readPtr, &fl);
        assert(readPtr == writePtr);
        
        writePtr = Message::write(writePtr, dbl);
        readPtr = Message::read(readPtr, &dbl);
        assert(readPtr == writePtr);
        
        assert(dbl = (double)value);
        assert(uint64 = (uint64_t)value);
        assert(int64 = (int64_t)value);
        
        char* start = writePtr;
        writePtr = Message::writeCompact(writePtr, uint64, 9);
        readPtr = Message::readCompact(readPtr, &uint64, 9);
        
        if (uint64 < 254) {
            assert(writePtr - start == 1);
        } else if (uint64 < (1 << 16)) {
            assert(writePtr - start == 3);
        } else {
            assert(writePtr - start == 9);
        }
        
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
        
        // standard case with enough space and termination
        writePtr = data;
        readPtr = data;
        readFrom = "This is foo";
        writePtr = Message::write(writePtr, readFrom);
        assert(writePtr == data + readFrom.size() + 1);
        
        readPtr = Message::read(readPtr, writeTo, 100);
        assert(readPtr == data + readFrom.size() + 1);
        assert(readFrom == writeTo);
        
        // enough space but no termination
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
    free(data);
    return true;
}

bool testCompression() {
    std::string test1;
    for (int i = 0; i < 20; i++)
        test1 += "This is some test right here!";
    
    {
        /**
         * Test compression with a context
         * This will compress all of the message
         */
        
        std::list<std::pair<std::pair<size_t, size_t>, std::pair<char*, char*> > > data;
        size_t iterations = 10;
        
        std::string procUUID = "896e8001-6389-4543-a5d7-d7ae745900a2";
        std::string hostUUID = "affa8baa-0c9a-4f1e-a08e-c87847fb61ba";
        std::string pubUUID  = "f56fbaaf-e7be-4d80-a67b-3f712961b258";
        
        void* ctx1 = Message::createCompression();
        for (size_t i = 0; i < iterations; i++) {
            
            Message msg(test1.data(), test1.size());
            msg.putMeta("um.pub", pubUUID);
            msg.putMeta("um.proc", procUUID);
            msg.putMeta("um.host", hostUUID);
            
            size_t headerSize  = msg.getCompressBounds("lz4", ctx1, Message::HEADER);
            size_t payloadSize = msg.getCompressBounds("lz4", ctx1, Message::PAYLOAD);
            
            char* onwireHeader  = (char*)malloc(headerSize);
            char* onwirePayload = (char*)malloc(payloadSize);
            
            headerSize  = msg.compress("lz4", ctx1, onwireHeader, headerSize, Message::HEADER);
            payloadSize = msg.compress("lz4", ctx1, onwirePayload, payloadSize, Message::PAYLOAD);
            data.push_back(std::make_pair(std::make_pair(headerSize, payloadSize), std::make_pair(onwireHeader, onwirePayload)));
            
        }
        Message::freeCompression(ctx1);
        
        void* ctx2 = Message::createCompression();
        for (size_t i = 0; i < iterations; i++) {
            std::pair<std::pair<size_t, size_t>, std::pair<char*, char*> > msgData = data.front();
            data.pop_front();
            
            size_t headerSize = msgData.first.first;
            size_t payloadSize = msgData.first.second;
            char* onwireHeader  = msgData.second.first;
            char* onwirePayload = msgData.second.second;
            
            std::cout << headerSize << ":" << payloadSize << std::endl;
            
            Message msg("lz4", ctx2, onwireHeader, headerSize, onwirePayload, payloadSize);
//            msg.uncompress("lz4", ctx2, onwireHeader, headerSize, Message::HEADER);
//            msg.uncompress("lz4", ctx2, onwirePayload, payloadSize, Message::PAYLOAD);
            //std::cout << std::string(msg.data(), msg.size()) << std::endl;
            
            assert(msg.getMeta("um.pub") == pubUUID);
            assert(msg.getMeta("um.proc") == procUUID);
            assert(msg.getMeta("um.host") == hostUUID);
            assert(msg.size() == test1.size());
            
            assert(md5(std::string(msg.data(), msg.size())) == md5(test1));
            
            free(onwireHeader);
            free(onwirePayload);
        }
        Message::freeCompression(ctx2);
    }

    {
        /**
         * Test a simple publisher with stateless compression
         */
        Node pubNode;
        PublisherConfigTCP pubConfig("bar");
#ifdef BUILD_WITH_COMPRESSION_LEVEL_LZ4
        pubConfig.enableCompression("lz4");
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_MINIZ)
        pubConfig.enableCompression("miniz");
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_FASTLZ)
        pubConfig.enableCompression("fastlz");
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
        nrReceptions = 0;
        nrMissing = 0;
        bytesRecvd = 0;

        for (int j = 0; j < iterations; j++) {
            char* buffer = (char*)malloc(BUFFER_SIZE);
            memset(buffer, j, BUFFER_SIZE);
            
            Message* msg = new Message(buffer, BUFFER_SIZE, Message::ADOPT_DATA);
            msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
            msg->putMeta("seq",toStr(j));
            pub.send(msg);
            //Thread::sleepMs(10);
            delete msg;
        }
        Thread::sleepMs(500);
        delete testRecv;
    }
    
    {
        /**
         * Test a publisher with a stateful compression
         */
        Node pubNode;
        PublisherConfigTCP pubConfig("bar");
#ifdef BUILD_WITH_COMPRESSION_LEVEL_LZ4
        pubConfig.enableCompression("lz4", true);
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_MINIZ)
        pubConfig.enableCompression("miniz", true);
#elif defined(BUILD_WITH_COMPRESSION_LEVEL_FASTLZ)
        pubConfig.enableCompression("fastlz", true);
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
        nrReceptions = 0;
        nrMissing = 0;
        bytesRecvd = 0;

        for (int j = 0; j < iterations; j++) {
            char* buffer = (char*)malloc(BUFFER_SIZE);
            memset(buffer, j, BUFFER_SIZE);
            
            Message* msg = new Message(buffer, BUFFER_SIZE, Message::ADOPT_DATA);
            msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
            msg->putMeta("seq",toStr(j));
            pub.send(msg);
            //Thread::sleepMs(10);
            delete msg;
        }
        Thread::sleepMs(500);
        delete testRecv;

    }
    
    return true;
}

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

		char* buffer = (char*)malloc(BUFFER_SIZE);
		memset(buffer, 40, BUFFER_SIZE);

		int iterations = 1000;

		for (int j = 0; j < iterations; j++) {
			Message* msg = new Message(buffer, BUFFER_SIZE);
			msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
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
        std::cout << "expected " << iterations << " messages, received " << nrReceptions << std::endl;
		assert(nrReceptions == iterations);
		assert(bytesRecvd == nrReceptions * BUFFER_SIZE);

		subNode.removeSubscriber(sub);
		pubNode.removePublisher(pub);

	}
	return true;
}


int main(int argc, char** argv, char** envp) {
	if (!testByteWriting())
		return EXIT_FAILURE;
	if (!testCompression())
		return EXIT_FAILURE;
	if (!testMessageTransmission())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
