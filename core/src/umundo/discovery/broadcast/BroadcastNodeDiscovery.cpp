#include "umundo/discovery/broadcast/BroadcastNodeDiscovery.h"
#include <zmq.h>

namespace umundo {

/**
 * Have a look at https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L433 to
 * see how to setup UDP sockets for broadcast.
 *
 * Receiving Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L700
 * Sending Broadcast messages: https://github.com/zeromq/czmq/blob/master/src/zbeacon.c#L751
 *
 * Pass -DDISC_BROADCAST=ON to cmake to enable this implementation.
 *
 * I'd like not to edit the behaviour of the ZeroMQNode, just pass whatever you
 * need to know on the remote end as part of the broadcast message please.
 */

BroadcastNodeDiscovery::BroadcastNodeDiscovery() {
	/**
	 * This is called twice overall, once for the prototype in the factory and
	 * once for the instance that is actually used.
	 */
}

BroadcastNodeDiscovery::~BroadcastNodeDiscovery() {
}

shared_ptr<BroadcastNodeDiscovery> BroadcastNodeDiscovery::getInstance() {
	if (_instance.get() == NULL) {
		_instance = shared_ptr<BroadcastNodeDiscovery>(new BroadcastNodeDiscovery());
		_instance->start();
	}
	return _instance;
}
shared_ptr<BroadcastNodeDiscovery> BroadcastNodeDiscovery::_instance;

shared_ptr<Implementation> BroadcastNodeDiscovery::create() {
	return getInstance();
}

void BroadcastNodeDiscovery::init(shared_ptr<Configuration>) {
	/*
	 * This is where you can setup the non-prototype instance, i.e. not the one
	 * in the factory, but the one created from the prototype.
	 *
	 * The configuration object is not used and empty.
	 */
}

void BroadcastNodeDiscovery::suspend() {
}

void BroadcastNodeDiscovery::resume() {
}

void BroadcastNodeDiscovery::add(NodeImpl* node) {
	/**
	 * umundo.core asks you to make the given node discoverable
	 */
}

void BroadcastNodeDiscovery::remove(NodeImpl* node) {
	/**
	 * umundo.core asks you to announce the removal of the given node
	 */
}

void BroadcastNodeDiscovery::browse(shared_ptr<NodeQuery> discovery) {
	/**
	 * Another node wants to browse for nodes
	 */
}

void BroadcastNodeDiscovery::unbrowse(shared_ptr<NodeQuery> discovery) {
	/**
	 * Another node no longer wants to browse for nodes
	 */
}

void BroadcastNodeDiscovery::run() {
	// your very own thread!
}

}
