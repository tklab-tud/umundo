#include "umundo/common/Factory.h"
#include "umundo/common/Message.h"
#include "umundo/connection/Node.h"
#include "umundo/discovery/Discovery.h"

using namespace umundo;

bool testInstantiation() {
	Node node1;
	Node node2;

	Discovery disc(Discovery::MDNS);

	disc.add(node1);
	disc.add(node2);

	RTPPublisherConfig pubConfig(8000, 166, 0);		//PCMU data with sample rate of 8000Hz and 20ms payload per rtp packet (166 samples)
	Publisher rtpPub(Publisher::RTP, "foo", &pubConfig);
	RTPSubscriberConfig subConfig;
	Subscriber rtpSub(Subscriber::RTP, "foo", &subConfig);

	node1.addSubscriber(rtpSub);
	node2.addPublisher(rtpPub);

	rtpPub.waitForSubscribers(1);

	Thread::sleepMs(30000);

	return true;
}

int main(int argc, char** argv) {
	setenv("UMUNDO_LOGLEVEL", "4", 1);
	if (!testInstantiation())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;

}