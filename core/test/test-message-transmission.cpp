#include "umundo/core.h"
#include "umundo/util.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024*1024

using namespace umundo;
using namespace std;

static int nrReceptions = 0;
static int bytesRecvd = 0;
static int nrMissing = 0;
static string hostId;

class TestReceiver : public Receiver {
	void receive(Message* msg) {
		// std::cout << "md5: '" << msg->getMeta("md5") << "'" << std::endl;
		// std::cout << "md5: '" << md5(msg->data(), msg->size()) << "'" << std::endl;
		// std::cout << "equals: " << msg->getMeta("md5").compare(md5(msg->data(), msg->size())) << std::endl;
		if (msg->size() > 0)
			assert(msg->getMeta("md5").compare(md5(msg->data(), msg->size())) == 0);
		if (nrReceptions + nrMissing == strTo<int>(msg->getMeta("seq")))
			std::cout << ".";
		while (nrReceptions + nrMissing < strTo<int>(msg->getMeta("seq"))) {
			nrMissing++;
			std::cout << "F" << nrReceptions + nrMissing << std::endl;
		}
		nrReceptions++;
		bytesRecvd += msg->size();
	}
};

bool testMessageTransmission() {
	hostId = Host::getHostId();
	for (int i = 0; i < 2; i++) {
		nrReceptions = 0;
		bytesRecvd = 0;
    
		Node* pubNode = new Node(hostId + "foo");
		Publisher* pub = new Publisher("foo");
		pubNode->addPublisher(pub);
    
    TestReceiver* testRecv = new TestReceiver();
		Node* subNode = new Node(hostId + "foo");
		Subscriber* sub = new Subscriber("foo", testRecv);
    sub->setReceiver(testRecv);
		subNode->addSubscriber(sub);
    
		pub->waitForSubscribers(1);
		assert(pub->waitForSubscribers(0) == 1);
    
		char* buffer = (char*)malloc(BUFFER_SIZE);
		memset(buffer, 40, BUFFER_SIZE);
    
		for (int j = 0; j < 100; j++) {
			Message* msg = new Message(Message(buffer, BUFFER_SIZE));
			msg->putMeta("md5", md5(buffer, BUFFER_SIZE));
			msg->putMeta("seq",toStr(j));
			pub->send(msg);
			delete msg;
		}
    
		// wait until all messages are delivered
		Thread::sleepMs(500);
    
		// sometimes there is some weird latency
		if (nrReceptions < 100)
			Thread::sleepMs(2000);
    
		std::cout << "expected 100 messages, received " << nrReceptions << std::endl;
		assert(nrReceptions == 100);
		assert(bytesRecvd == nrReceptions * BUFFER_SIZE);
    
		int iterations = 5;
		while(iterations-- > 0) {
			nrReceptions = 0;
			uint64_t now = Thread::getTimeStampMs();
			while (now > Thread::getTimeStampMs() - 10) {
				Message* msg2 = new Message();
				pub->send(msg2);
				Thread::yield();
				delete msg2;
			}
			Thread::sleepMs(100);
			std::cout << nrReceptions * 100 << " messages per second" << std::endl;
		}
    
		// test explicit sub removal
		subNode->removeSubscriber(sub);
		for (int k = 0; k < 8; k++) {
			if (pub->waitForSubscribers(0) == 0)
				break;
			Thread::sleepMs(50);
		}
		assert(pub->waitForSubscribers(0) == 0);
    
#if 0
		// The node still tells everyone else that the subscriber unsubscribed
		subNode->addSubscriber(sub);
		pub->waitForSubscribers(1);
    
		// test node destruction
		delete subNode;
		for (int k = 0; k < 8; k++) {
			if (pub->waitForSubscribers(0) == 0)
				break;
			Thread::sleepMs(50);
		}
		assert(pub->waitForSubscribers(0) == 0);
#endif
		pubNode->removePublisher(pub);
    
		delete pubNode;
		delete pub;
		delete sub;
    
	}
  return true;
}


int main(int argc, char** argv, char** envp) {
  if (!testMessageTransmission())
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
