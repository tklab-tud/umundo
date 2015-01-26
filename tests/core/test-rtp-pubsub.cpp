#include "umundo/core/Factory.h"
#include "umundo/core/Message.h"
#include "umundo/core/connection/Node.h"
#include "umundo/core/discovery/Discovery.h"

using namespace umundo;

bool testInstantiation() {
	Node node1;
	Node node2;

	Discovery disc(Discovery::MDNS);

	disc.add(node1);
	disc.add(node2);

	PublisherConfigRTP pubConfig("foo");
	pubConfig.setTimestampIncrement(166);
	Publisher rtpPub(&pubConfig);
	
	SubscriberConfigRTP subConfig("foo");
	Subscriber rtpSub(&subConfig);

	node1.addSubscriber(rtpSub);
	node2.addPublisher(rtpPub);

	rtpPub.waitForSubscribers(1);

	return true;
}

int main(int argc, char** argv) {
	setenv("UMUNDO_LOGLEVEL", "4", 1);
	if (!testInstantiation())
		return EXIT_FAILURE;
	return EXIT_SUCCESS;

}