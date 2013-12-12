#ifndef BROADCASTDISCOVERY_H_ZZW33B2N
#define BROADCASTDISCOVERY_H_ZZW33B2N

#include "umundo/common/Common.h"
#include "umundo/thread/Thread.h"
#include "umundo/discovery/Discovery.h"


namespace umundo {

class NodeQuery;

class BroadcastDiscoveryOptions : public Options {
public:
	enum Protocol {
		TCP, UDP
	};
	
	BroadcastDiscoveryOptions() {
		options["broadcast.domain"] = "local.";
		options["broadcast.protocol"] = "tcp";
		options["broadcast.serviceType"] = "umundo";
	}
	
	std::string getType() {
		return "broadcast";
	}
	
	void setDomain(const std::string& domain) {
		options["broadcast.domain"] = domain;
	}
	
	void setProtocol(Protocol protocol) {
		switch (protocol) {
			case UDP:
				options["broadcast.protocol"] = "udp";
				break;
			case TCP:
				options["broadcast.protocol"] = "tcp";
				break;
				
			default:
				break;
		}
	}
	
	void setServiceType(const std::string& serviceType) {
		options["broadcast.serviceType"] = serviceType;
	}
	
};

/**
 * Concrete discovery implementor for Broadcast (bridge pattern).
 *
 * This class is a concrete implementor (in the bridge pattern sense) for the Discovery subsystem.
 */
class DLLEXPORT BroadcastDiscovery : public DiscoveryImpl, public Thread {
public:
	virtual ~BroadcastDiscovery();
	static boost::shared_ptr<BroadcastDiscovery> getInstance();  ///< Return the singleton instance.

	boost::shared_ptr<Implementation> create();
	void init(Options*);
	void suspend();
	void resume();

	void advertise(const EndPoint& node);
	void add(Node& node);
	void unadvertise(const EndPoint& node);
	void remove(Node& node);

	void browse(ResultSet<EndPoint>* query);
	void unbrowse(ResultSet<EndPoint>* query);

	std::vector<EndPoint> list();

	void run();

protected:
	BroadcastDiscovery();
	static boost::shared_ptr<BroadcastDiscovery> _instance;  ///< The singleton instance.

	friend class Factory;
};

}


#endif /* end of include guard: BROADCASTDISCOVERY_H_ZZW33B2N */
