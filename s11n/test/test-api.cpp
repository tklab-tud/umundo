#include "umundo/s11n.h"
#include "umundo/s11n/protobuf/PBSerializer.h"
#include "umundo/core.h"
#include "Test1.pb.h"

using namespace umundo;

class TestTypedReceiver : public TypedReceiver {
public:
	virtual ~TestTypedReceiver() {}
	void receive(void* obj, Message* msg) {
		std::cout << msg->getMeta("um.s11n.type") << ": ";
		if (msg->getMeta("um.s11n.type").compare("Test1Msg") == 0) {
			// we got an explicit type from tSub->registerType
			Test1Msg* tMsg = (Test1Msg*)obj;
			std::cout << tMsg->doubletype() << ": " << tMsg->stringtype() << std::endl;
			delete tMsg;
		} else if (msg->getMeta("um.s11n.type").compare("google::protobuf::Message") == 0) {
			// we only have the descriptor from PBSerializer::addProto
			google::protobuf::Message* tMsg = (google::protobuf::Message*)obj;
			tMsg->PrintDebugString();
			delete tMsg;
		}
	}
};

int main(int argc, char** argv) {
	Test1Msg test1MsgType;
	for (int i = 0; i < 1000; i++) {
		setenv("UMUNDO_LOGLEVEL", "4", 1);

		Node mainNode;
		Node otherNode;

		Discovery disc(Discovery::MDNS);
		disc.add(mainNode);
		disc.add(otherNode);

		TestTypedReceiver* tts = new TestTypedReceiver();
		TypedPublisher tPub("fooChannel");
		TypedSubscriber tSub("fooChannel", tts);

		//  PBSerializer::addProto("/Users/sradomski/Documents/TK/Code/umundo/s11n/test/proto/Test1.proto");

		tSub.registerType("Test1Msg", &test1MsgType);
		tPub.registerType("Test1Msg", &test1MsgType);

		mainNode.addPublisher(tPub);
		otherNode.addSubscriber(tSub);

		// try a typed message for atomic types
		Test1Msg* tMsg1 = new Test1Msg();
		tPub.waitForSubscribers(1);

		int iterations = 1000;
		while(iterations--) {
			tMsg1->set_doubletype(iterations);
			tMsg1->set_stringtype("foo");
			tPub.sendObj("Test1Msg", tMsg1);
		}
		delete tMsg1;

		Thread::sleepMs(100);

		// this is important as we will access the delete tts otherwise
		tSub.setReceiver(NULL);
		delete tts;
	}
}