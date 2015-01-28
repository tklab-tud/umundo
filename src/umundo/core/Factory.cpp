/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

#include "umundo/core/Factory.h"
#include "umundo/core/UUID.h"
#include "umundo/core/Host.h"

#include "umundo/core/connection/zeromq/ZeroMQNode.h"
#include "umundo/core/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/core/connection/zeromq/ZeroMQSubscriber.h"
#include "umundo/config.h"

#ifdef DISC_BONJOUR
#include "umundo/core/discovery/mdns/bonjour/BonjourDiscoverer.h"
#endif

#ifdef DISC_AVAHI
#include "umundo/core/discovery/mdns/avahi/AvahiNodeDiscovery.h"
#endif

#ifdef DISC_BROADCAST
#include "umundo/core/discovery/BroadcastDiscovery.h"
#endif

#if (defined DISC_AVAHI || defined DISC_BONJOUR)
#include "umundo/core/discovery/MDNSDiscovery.h"
#endif

#ifdef NET_RTP
#include "umundo/core/connection/rtp/RTPPublisher.h"
#include "umundo/core/connection/rtp/RTPSubscriber.h"
#endif

#if !(defined NET_ZEROMQ)
#error "No discovery implementation choosen"
#endif



#ifdef BUILD_DEBUG
#include <signal.h>
#endif

namespace umundo {

std::string procUUID;
std::string hostUUID;
Factory* Factory::_instance = NULL;

Factory* Factory::getInstance() {
	if (Factory::_instance == NULL) {
		procUUID = UUID::getUUID();
		hostUUID = Host::getHostId();
#ifdef BUILD_DEBUG
		Debug::abortWithStackTraceOnSignal(SIGSEGV);
#ifdef UNIX
		Debug::abortWithStackTraceOnSignal(SIGBUS);
		Debug::abortWithStackTraceOnSignal(SIGILL);
		Debug::abortWithStackTraceOnSignal(SIGFPE);
		Debug::abortWithStackTraceOnSignal(SIGABRT);
#endif
#endif
		Factory::_instance = new Factory();
	}
	return Factory::_instance;
}

Factory::Factory() {
	_prototypes["pub.zmq"] = new ZeroMQPublisher();
	_prototypes["sub.zmq"] = new ZeroMQSubscriber();
	_prototypes["node.zmq"] = new ZeroMQNode();
#if (defined DISC_AVAHI || defined DISC_BONJOUR)
	_prototypes["discovery.mdns"] = new MDNSDiscovery();
#endif
#ifdef DISC_BONJOUR
	_prototypes["discovery.mdns.impl"] = new BonjourDiscoverer();
#endif
#ifdef DISC_AVAHI
	_prototypes["discovery.mdns.impl"] = new AvahiNodeDiscovery();
#endif
#ifdef DISC_BROADCAST
	_prototypes["discovery.broadcast"] = new BroadcastDiscovery();
#endif
#ifdef NET_RTP
	_prototypes["pub.rtp"] = new RTPPublisher();
	_prototypes["sub.rtp"] = new RTPSubscriber();
#endif
}

SharedPtr<Implementation> Factory::create(const std::string& name) {
	Factory* factory = getInstance();
	RScopeLock lock(factory->_mutex);
	if (factory->_prototypes.find(name) == factory->_prototypes.end()) {
		UM_LOG_WARN("No prototype registered for %s", name.c_str());
		return SharedPtr<Implementation>();
	}
	SharedPtr<Implementation> implementation = factory->_prototypes[name]->create();
	factory->_implementations.push_back(implementation);
	return implementation;
}

void Factory::registerPrototype(const std::string& name, Implementation* prototype) {
	Factory* factory = getInstance();
	RScopeLock lock(factory->_mutex);
	if (factory->_prototypes.find(name) != factory->_prototypes.end()) {
		UM_LOG_WARN("Overwriting existing prototype for %s", name.c_str());
	}
	factory->_prototypes[name] = prototype;
}

/**
 * Iterate created instances from newest to oldest and send suspend request.
 */
void Factory::suspendInstances() {
	Factory* factory = getInstance();
	UMUNDO_LOCK(factory->_mutex);
	std::vector<WeakPtr<Implementation> >::reverse_iterator implIter = factory->_implementations.rbegin();
	while(implIter != factory->_implementations.rend()) {
		SharedPtr<Implementation> implementation = implIter->lock();
		if (implementation.get() != NULL) {
			implementation->suspend();
		} else {
			// I have not found a way to savely remove a reverse iterator - just rely on resumeInstances
		}
		implIter++;
	}
	UMUNDO_UNLOCK(factory->_mutex);
}

/**
 * Iterate created instances from oldest to newest and send resume request.
 */
void Factory::resumeInstances() {
	return;
	Factory* factory = getInstance();
	UMUNDO_LOCK(factory->_mutex);
	std::vector<WeakPtr<Implementation> >::iterator implIter = factory->_implementations.begin();
	while(implIter != factory->_implementations.end()) {
		SharedPtr<Implementation> implementation = implIter->lock();
		if (implementation.get() != NULL) {
			implementation->resume();
			implIter++;
		} else {
			// weak pointer points to deleted object
			implIter = factory->_implementations.erase(implIter);
		}
	}
	UMUNDO_UNLOCK(factory->_mutex);
}

}