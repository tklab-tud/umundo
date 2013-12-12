#include "umundo/discovery/BroadcastDiscovery.h"
#include <zmq.h>

namespace umundo {

/**
 * Have a look at https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L433 to
 * see how to setup UDP sockets for broadcast.
 *
 * Receiving Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L700
 * Sending Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L751
 */

BroadcastDiscovery::BroadcastDiscovery() {
	/**
	 * This is called twice overall, once for the prototype in the factory and
	 * once for the instance that is actually used.
	 */
}

BroadcastDiscovery::~BroadcastDiscovery() {
}

boost::shared_ptr<BroadcastDiscovery> BroadcastDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		_instance = boost::shared_ptr<BroadcastDiscovery>(new BroadcastDiscovery());
		_instance->start();
	}
	return _instance;
}
boost::shared_ptr<BroadcastDiscovery> BroadcastDiscovery::_instance;

boost::shared_ptr<Implementation> BroadcastDiscovery::create() {
	return getInstance();
}

void BroadcastDiscovery::init(Options*) {
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

void BroadcastDiscovery::browse(ResultSet<EndPoint>* query) {
	/**
	 * Another node wants to browse for nodes
	 */
}

void BroadcastDiscovery::unbrowse(ResultSet<EndPoint>* query) {
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
