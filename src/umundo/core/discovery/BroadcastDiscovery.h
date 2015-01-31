#ifndef BROADCASTDISCOVERY_H_ZZW33B2N
#define BROADCASTDISCOVERY_H_ZZW33B2N

#include "umundo/core/Common.h"
#include "umundo/core/thread/Thread.h"
#include "umundo/core/discovery/Discovery.h"

#if (defined (__UNIX__) || defined (__APPLE__))
#include <netinet/in.h>
#include <netdb.h>
#endif

#if (defined (_WIN32))
#include <winsock2.h>
//#include <ws2tcpip.h>            //  getnameinfo ()
//#include <iphlpapi.h>            //  GetAdaptersAddresses ()
//#include <sys/types.h>
#endif

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
class UMUNDO_API BroadcastDiscovery : public DiscoveryImpl, public Thread {
public:
	virtual ~BroadcastDiscovery();
	static SharedPtr<BroadcastDiscovery> getInstance();  ///< Return the singleton instance.

	SharedPtr<Implementation> create();
	void init(const Options*);
	void suspend();
	void resume();

	void advertise(const EndPoint& node);
	void add(Node& node);
	void unadvertise(const EndPoint& node);
	void remove(Node& node);

	void browse(ResultSet<ENDPOINT_RS_TYPE>* query);
	void unbrowse(ResultSet<ENDPOINT_RS_TYPE>* query);

	std::vector<EndPoint> list();

	void run();

protected:
	BroadcastDiscovery();
	static SharedPtr<BroadcastDiscovery> _instance;  ///< The singleton instance.

private:
#if 0
	void static setupUDPSocket();

#if !defined (_WIN32)
#define SOCKET         int
#define closesocket    close
#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1
#endif

	static SOCKET udpSocket;
	static sockaddr_in bCastAddress;
	static char hostName[NI_MAXHOST];
	static RMutex globalMutex;
#endif

	friend class Factory;
};

}


#endif /* end of include guard: BROADCASTDISCOVERY_H_ZZW33B2N */
