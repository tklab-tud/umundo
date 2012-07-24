#include "umundo/core.h"
#include "umundo/util.h"
#include <iostream>
#include <stdio.h>

#define BUFFER_SIZE 1024*1024

using namespace umundo;

static int nrReceptions = 0;
static int bytesRecvd = 0;


class TestReceiver : public Receiver {
	void receive(Message* msg) {
		// std::cout << "md5: '" << msg->getMeta("md5") << "'" << std::endl;
		// std::cout << "md5: '" << md5(msg->data(), msg->size()) << "'" << std::endl;
		// std::cout << "equals: " << msg->getMeta("md5").compare(md5(msg->data(), msg->size())) << std::endl;
		if (msg->size() > 0)
			assert(msg->getMeta("md5").compare(md5(msg->data(), msg->size())) == 0);
		nrReceptions++;
		bytesRecvd += msg->size();
	}
};

int main(int argc, char** argv, char** envp) {
	for (int i = 0; i < 2; i++) {
		nrReceptions = 0;
		bytesRecvd = 0;

		Node* pubNode = new Node("foo");
		Publisher* pub = new Publisher("foo");
		pubNode->addPublisher(pub);

		Node* subNode = new Node("foo");
		Subscriber* sub = new Subscriber("foo", new TestReceiver());
		subNode->addSubscriber(sub);

		pub->waitForSubscribers(1);

		char* buffer = (char*)malloc(BUFFER_SIZE);
		memset(buffer, 40, BUFFER_SIZE);

		for (int i = 0; i < 100; i++) {
			Message* msg = new Message(Message(buffer, BUFFER_SIZE));
			msg->setMeta("md5", md5(buffer, BUFFER_SIZE));
			pub->send(msg);
			delete msg;
		}

		Thread::sleepMs(100);
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

		pubNode->removePublisher(pub);
		subNode->removeSubscriber(sub);

		delete subNode;
		delete pubNode;
		delete pub;
		delete sub;

	}
	return EXIT_SUCCESS;
}
