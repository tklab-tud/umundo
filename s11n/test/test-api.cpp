//#include "um_message.pb.h"
//#include "um_typed_message.pb.h"
#include "interfaces/protobuf/custom_typed_message.pb.h"
#include "interfaces/protobuf/typed_message.pb.h"
#include "interfaces/protobuf/um_person.pb.h"
#include "umundo/s11n.h"
#include "umundo/core.h"

#ifdef __GNUC__
#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>

void handler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, 2);
	exit(1);
}
#endif

using namespace umundo;

class TestTypedReceiver : public TypedReceiver {
	void receive(void* obj, Message* msg) {
		if (msg->getMeta("type").length() == 0) {
			// standard message
			PBTypedMessage* tMsg = (PBTypedMessage*)obj;
			if (tMsg->has_intpayload()) {
				std::cout << "Received int[]: ";
				for (int i = 0; i < tMsg->intpayload().payload_size(); i++) {
					std::cout << tMsg->intpayload().payload(i) << ", ";
				}
				std::cout << std::endl;
			}
		} else if (msg->getMeta("type").compare("person") == 0) {
			std::cout << "received " << msg->getMeta("type") << " " << ((Person*)obj)->name() << std::endl;
		}
	}
};

int main(int argc, char** argv) {
#ifdef __GNUC__
	signal(SIGSEGV, handler);   // install our handler
#endif

	Node* mainNode = new Node();
	Node* otherNode = new Node();
	TestTypedReceiver* tts = new TestTypedReceiver();
	TypedPublisher* tPub = new TypedPublisher("fooChannel");
	TypedSubscriber* tSub = new TypedSubscriber("fooChannel", tts);

	mainNode->addPublisher(tPub);
	otherNode->addSubscriber(tSub);

	// try a typed message for atomic types
	PBTypedMessage* tMsg = new PBTypedMessage();
	tSub->registerType("PBTypedMessage", new PBTypedMessage());
	for (int i = 0; i < 32; i++) {
		tMsg->mutable_intpayload()->add_payload(i);
	}

	while(true) {
		Thread::sleepMs(1000);
		tPub->sendObj("PBTypedMessage", tMsg);
	}


//  tSub->registerType("person", new Person());
//  tPub->registerType("person", new Person());
//
//
//	Person* person = new Person();
//	person->set_id(234525);
//	person->set_name("Captain FooBar");
//  while(true) {
//    Thread::sleepMs(1000);
//    tPub->sendObj("person", person);
//  }
}