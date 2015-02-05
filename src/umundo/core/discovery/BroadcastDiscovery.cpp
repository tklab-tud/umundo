#include "umundo/core/discovery/BroadcastDiscovery.h"

namespace umundo {

/**
 * Have a look at https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L81 to
 * see how to setup UDP sockets for broadcast.
 *
 * Receiving Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L700
 * Sending Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L751
 */

#if 0
SOCKET BroadcastDiscovery::udpSocket = 0;
RMutex BroadcastDiscovery::globalMutex;

sockaddr_in BroadcastDiscovery::bCastAddress;
char BroadcastDiscovery::hostName[NI_MAXHOST];
#endif

BroadcastDiscovery::BroadcastDiscovery() {
	/**
	 * This is called twice overall, once for the prototype in the factory and
	 * once for the instance that is actually used.
	 */
}

BroadcastDiscovery::~BroadcastDiscovery() {
}

#if 0
void BroadcastDiscovery::setupUDPSocket() {
	RScopeLock(globalMutex);
	if (udpSocket == 0) {
		int enable = 1;

		udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (udpSocket == INVALID_SOCKET)
			return;

		if (setsockopt (udpSocket, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(enable)) == SOCKET_ERROR) {
			//error
		}
		if (setsockopt (udpSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&enable, sizeof(enable)) == SOCKET_ERROR) {
			//error
		}

#if defined (SO_REUSEPORT)
		if (setsockopt (udpSocket, SOL_SOCKET, SO_REUSEPORT, (char*)&enable, sizeof(enable)) == SOCKET_ERROR) {
			//error
		}
#endif
		in_addr_t bindTo = INADDR_ANY;
		in_addr_t sendTo = INADDR_BROADCAST;

		bCastAddress.sin_family = AF_INET;
		bCastAddress.sin_port = htons(43005);
		bCastAddress.sin_addr.s_addr = sendTo;

		sockaddr_in address = bCastAddress;
		address.sin_addr.s_addr = bindTo;

#if (defined (__WINDOWS__))
		sockaddr_in sockaddr = address;
#elif (defined (__APPLE__))
		sockaddr_in sockaddr = bCastAddress;
		sockaddr.sin_addr.s_addr = htons (INADDR_ANY);
#else
		sockaddr_in sockaddr = bCastAddress;
#endif

		if (bind (udpSocket, (struct sockaddr *) &sockaddr, sizeof (sockaddr_in))) {
			// error
		}

		getnameinfo ((struct sockaddr *) &address, sizeof (sockaddr_in), hostName, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

	}
}
#endif

SharedPtr<BroadcastDiscovery> BroadcastDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		// setup UDP socket when th first instance is required
		//setupUDPSocket();

		_instance = SharedPtr<BroadcastDiscovery>(new BroadcastDiscovery());
		_instance->start();
	}
	return _instance;
}
SharedPtr<BroadcastDiscovery> BroadcastDiscovery::_instance;

SharedPtr<Implementation> BroadcastDiscovery::create() {
	return getInstance();
}

void BroadcastDiscovery::init(const Options*) {
	/*
	 * This is where you can setup the non-prototype instance, i.e. not the one
	 * in the factory, but the one created from the prototype.
	 *
	 * The options object is not used and empty.
	 */
}

void BroadcastDiscovery::suspend() {
}

void BroadcastDiscovery::resume() {
}

void BroadcastDiscovery::advertise(const EndPoint& node) {
	/**
	 * umundo.core asks you to make the given node discoverable
	 */
}

void BroadcastDiscovery::add(Node& node) {
}

void BroadcastDiscovery::unadvertise(const EndPoint& node) {
	/**
	 * umundo.core asks you to announce the removal of the given node
	 */
}

void BroadcastDiscovery::remove(Node& node) {
}

void BroadcastDiscovery::browse(ResultSet<ENDPOINT_RS_TYPE>* query) {
	/**
	 * Another node wants to browse for nodes
	 */
}

void BroadcastDiscovery::unbrowse(ResultSet<ENDPOINT_RS_TYPE>* query) {
	/**
	 * Another node no longer wants to browse for nodes
	 */
}

std::vector<EndPoint> BroadcastDiscovery::list() {
	return std::vector<EndPoint>();
}

void BroadcastDiscovery::run() {
	// your very own thread!
}

}
