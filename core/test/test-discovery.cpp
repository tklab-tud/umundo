#define protected public
#define private public
#include "umundo/core.h"
#include "umundo/discovery/MDNSDiscovery.h"
#include "umundo/discovery/BroadcastDiscovery.h"
#include <iostream>
#include <stdio.h>

#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"

using namespace umundo;

static std::string hostId;
static Monitor monitor;
static Mutex mutex;

class EndPointResultSet : public ResultSet<EndPoint> {
public:
	void added(EndPoint node) {
		assert(_endPoints.find(node) == _endPoints.end());
		_endPoints.insert(node);
	}
	void removed(EndPoint node) {
		assert(_endPoints.find(node) != _endPoints.end());
		_endPoints.erase(node);
	}
	void changed(EndPoint node) {
		assert(_endPoints.find(node) != _endPoints.end());
	}

	std::set<EndPoint> _endPoints;
};

class MDNSAdResultSet : public ResultSet<MDNSAd*> {
public:
	void added(MDNSAd* node) {
		assert(_endPoints.find(node) == _endPoints.end());
		_endPoints.insert(node);
	}
	void removed(MDNSAd* node) {
		assert(_endPoints.find(node) != _endPoints.end());
		_endPoints.erase(node);
	}
	void changed(MDNSAd* node) {
		assert(_endPoints.find(node) != _endPoints.end());
	}

	std::set<MDNSAd*> _endPoints;
};

class DiscoveryObjectResultSet : public ResultSet<EndPoint> {
public:
	DiscoveryObjectResultSet(const std::string& type) : _type(type) {
	}
	void added(EndPoint node) {
		std::cout << "added '" << _type << "': " << node << std::endl;
	}
	void removed(EndPoint node) {
		std::cout << "removed '" << _type << "': " << node << std::endl;
	}
	void changed(EndPoint node) {
		std::cout << "changed '" << _type << "': " << node << std::endl;
	}

	std::string _type;
};

bool testDiscoveryObject() {
	// see https://developer.apple.com/library/mac/qa/qa1312/_index.html
	std::list<Discovery> discoveries;
	std::list<std::pair<std::string, MDNSDiscoveryOptions::Protocol> > services;
	services.push_back(std::make_pair("afpovertcp", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("nfs", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("webdav", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("ftp", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("ssh", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("eppc", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("http", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("telnet", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("printer", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("ipp", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("pdl-datastream", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("riousbprint", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("daap", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("dpap", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("ichat", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("airport", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("xserveraid", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("distcc", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("apple-sasl", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("workstation", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("servermgr", MDNSDiscoveryOptions::TCP));
	services.push_back(std::make_pair("raop", MDNSDiscoveryOptions::TCP));


	for (std::list<std::pair<std::string, MDNSDiscoveryOptions::Protocol> >::iterator svcIter = services.begin();
	        svcIter != services.end();
	        svcIter++) {
		MDNSDiscoveryOptions mdnsConfig;
		mdnsConfig.setServiceType(svcIter->first);
		mdnsConfig.setProtocol(svcIter->second);
		Discovery mdnsDisc(Discovery::MDNS, &mdnsConfig);

		DiscoveryObjectResultSet rs(svcIter->first);

		mdnsDisc.browse(&rs);
		discoveries.push_back(mdnsDisc);
	}

	tthread::this_thread::sleep_for(tthread::chrono::seconds(3));

	for (std::list<Discovery>::iterator discIter = discoveries.begin();
	        discIter != discoveries.end();) {
		discoveries.erase(discIter++);
	}

	return true;
}

bool testMulticastDNSDiscovery() {
	boost::shared_ptr<MDNSDiscoveryImpl> mdnsImpl = boost::static_pointer_cast<MDNSDiscoveryImpl>(Factory::create("discovery.mdns.impl"));

	std::string domain = "foo.local.";
	std::string regType = "_umundo._tcp.";

	MDNSAdResultSet* rs = new MDNSAdResultSet();

	MDNSQuery query;
	query.domain = domain;
	query.regType = regType;
	query.rs = rs;

	mdnsImpl->browse(&query);

	std::set<MDNSAd*> ads;
	int retries;
	int iterations = 3;
	int nrAds = 4;
	for (int i = 0; i < iterations; i++) {
		for (int j = 0; j < nrAds; j++) {
			MDNSAd* ad = new MDNSAd();
			ad->name = toStr(j);
			ad->regType = "_umundo._tcp.";
			ad->domain = "foo.local.";
			ad->port = i + 3000;

			mdnsImpl->advertise(ad);
			ads.insert(ad);
		}

		retries = 100;
		while(rs->_endPoints.size() != nrAds) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
		}

		std::set<MDNSAd*>::iterator adIter = ads.begin();
		while(adIter != ads.end()) {
			mdnsImpl->unadvertise(*adIter);
			delete *adIter;
			adIter++;
		}

		retries = 50;
		while(rs->_endPoints.size() != 0) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
		}

		ads.clear();

		std::cout << std::endl << std::endl;

	}
	mdnsImpl->unbrowse(&query);
	return true;
}

bool testEndpointDiscovery() {

	MDNSDiscoveryOptions mdnsOuterOpts;
	mdnsOuterOpts.setDomain("foo.local.");
	Discovery discOuter(Discovery::MDNS, &mdnsOuterOpts);

	EndPoint ep1("tcp://127.0.0.1:400");
	discOuter.advertise(ep1);

	int iterations = 4;
	int nrEndPoints = 3;
	for (int i = 0; i < iterations; ++i) {

		EndPointResultSet eprs;
		Discovery discInner(Discovery::MDNS);

		discInner.browse(&eprs);
		discOuter.browse(&eprs);

		std::set<EndPoint> endPoints;
		for (int j = 0; j < nrEndPoints; ++j) {
			std::stringstream epAddress;
			epAddress << "tcp://127.0.0.1:" << (i+j) % 4000 + 1000 ;
			EndPoint ep(epAddress.str());
			endPoints.insert(ep);
			discInner.advertise(ep);
			//			discOuter.advertise(ep);
		}

		int retries = 50;
		while(eprs._endPoints.size() != nrEndPoints + 1) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
		}

		std::set<EndPoint>::iterator epIter = endPoints.begin();
		while(epIter != endPoints.end()) {
			discInner.unadvertise(*epIter);
			epIter++;
		}

		discOuter.unbrowse(&eprs);
		endPoints.clear();

		std::cout << std::endl << std::endl;
	}
	return true;
}

bool testNodeDiscovery() {
	int retries;
	int iterations = 10;
	int nrNodes = 2;
	std::vector<EndPoint> foundNodes;
	for (int i = 0; i < iterations; ++i) {

		Discovery disc(Discovery::MDNS);

		// make sure there are no pending nodes
		retries = 100;
		foundNodes = disc.list();
		while(foundNodes.size() != 0) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
			foundNodes = disc.list();
		}
		assert(disc.list().size() == 0);

		std::set<Node> nodes;
		for (int j = 0; j < nrNodes; ++j) {
			Node node;
			disc.add(node);
			nodes.insert(node);
		}

		retries = 100;
		foundNodes = disc.list();
		while(foundNodes.size() != nrNodes) {
			Thread::sleepMs(100);
			if (retries-- == 0)
				assert(false);
			foundNodes = disc.list();
		}
		assert(disc.list().size() == nrNodes);

		std::map<std::string, NodeStub> peers;
		std::set<Node>::iterator nodeIter1 = nodes.begin();
		while(nodeIter1 != nodes.end()) {
			peers = const_cast<Node&>(*nodeIter1).connectedTo();
			std::set<Node>::iterator nodeIter2 = nodes.begin();
			while(nodeIter2 != nodes.end()) {
				// make sure everyone found exactly everyone else
				if (nodeIter1->getUUID() != nodeIter2->getUUID()) {
					retries = 200;
					while(peers.find(nodeIter2->getUUID()) == peers.end()) {
						Thread::sleepMs(100);
						if (retries-- == 0)
							assert(false);
						peers = const_cast<Node&>(*nodeIter1).connectedTo();
					}
				}
				nodeIter2++;
			}
			assert(peers.size() == nrNodes - 1);
			nodeIter1++;
		}
		std::set<Node>::iterator nodeIter = nodes.begin();
		while(nodeIter != nodes.end()) {
			disc.remove(const_cast<Node&>(*nodeIter));
			nodeIter++;
		}
		nodes.clear();

	}
	return true;
}

bool testPubSubConnections() {
	// test node / publisher / subscriber churn
	for (int i = 0; i < 10; i++) {
//		Thread::sleepMs(2000);
		Node node1;
		Node node2;

		node1.added(node2);
		node2.added(node1);

		Publisher pub("foo");
		node1.addPublisher(pub);

		for (int j = 0; j < 2; j++) {
			Subscriber sub1("foo", NULL, NULL);
			Subscriber sub2("foo", NULL, NULL);
			Subscriber sub3("foo", NULL, NULL);

			int subs = 0;
			(void)subs;

			subs = pub.waitForSubscribers(0);
			assert(subs == 0);

			Thread::sleepMs(300);

			node2.addSubscriber(sub1);
			std::cout << "Waiting for 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(1);
			assert(subs == 1);

			node2.addSubscriber(sub2);
			std::cout << "Waiting for 2nd subscriber" << std::endl;
			subs = pub.waitForSubscribers(2);
			assert(subs == 2);

			node2.addSubscriber(sub3);
			std::cout << "Waiting for 3rd subscriber" << std::endl;
			subs = pub.waitForSubscribers(3);
			assert(subs == 3);

			node2.removeSubscriber(sub1);
			Thread::sleepMs(300);
			std::cout << "Removed 1st subscriber" << std::endl;
			subs = pub.waitForSubscribers(0);
			assert(subs == 2);

			node2.removeSubscriber(sub2);
			std::cout << "Removed 2nd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 1);

			node2.removeSubscriber(sub3);
			std::cout << "Removed 3rd subscriber" << std::endl;
			Thread::sleepMs(300);
			subs = pub.waitForSubscribers(0);
			assert(subs == 0);

//			node1.removePublisher(pub);

			std::cout << "--- Successfully connected subscribers to publishers" << std::endl;

		}
	}
	return true;
}


int main(int argc, char** argv, char** envp) {
	setenv("UMUNDO_LOGLEVEL", "4", 1);
//	if (!testMulticastDNSDiscovery())
//		return EXIT_FAILURE;
//	if (!testEndpointDiscovery())
//		return EXIT_FAILURE;
//	if (!testDiscoveryObject())
//		return EXIT_FAILURE;
	if (!testNodeDiscovery())
		return EXIT_FAILURE;
//	if (!testPubSubConnections())
//		return EXIT_FAILURE;
	return EXIT_SUCCESS;


}
