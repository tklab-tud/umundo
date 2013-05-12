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

#include "umundo/common/Factory.h"
#include "umundo/common/UUID.h"

#include "umundo/connection/zeromq/ZeroMQNode.h"
#include "umundo/connection/zeromq/ZeroMQPublisher.h"
#include "umundo/connection/zeromq/ZeroMQSubscriber.h"
#include "umundo/config.h"

#ifdef DISC_BONJOUR
#include "umundo/discovery/bonjour/BonjourNodeDiscovery.h"
#endif

#ifdef DISC_AVAHI
#include "umundo/discovery/avahi/AvahiNodeDiscovery.h"
#endif

#if !(defined DISC_AVAHI || defined DISC_BONJOUR)
#error "No discovery implementation choosen"
#endif

#if !(defined NET_ZEROMQ)
#error "No discovery implementation choosen"
#endif

#ifdef BUILD_DEBUG
#include <signal.h>
#endif

namespace umundo {

string procUUID;
Factory* Factory::_instance = NULL;

Factory* Factory::getInstance() {
	if (Factory::_instance == NULL) {
		procUUID = UUID::getUUID();
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
		assert(_instance->_prototypes["publisher"] != NULL);
		assert(_instance->_prototypes["subscriber"] != NULL);
		assert(_instance->_prototypes["discovery"] != NULL);
		assert(_instance->_prototypes["node"] != NULL);
	}
	return Factory::_instance;
}

Factory::Factory() {
	_prototypes["publisher"] = new ZeroMQPublisher();
	_configures["publisher"] = new PublisherConfig();
	_prototypes["subscriber"] = new ZeroMQSubscriber();
	_configures["subscriber"] = new SubscriberConfig();
	_prototypes["node"] = new ZeroMQNode();
	_configures["node"] = new NodeConfig();
#ifdef DISC_BONJOUR
	_prototypes["discovery"] = new BonjourNodeDiscovery();
#endif
#ifdef DISC_AVAHI
	_prototypes["discovery"] = new AvahiNodeDiscovery();
#endif
}

shared_ptr<Configuration> Factory::config(string name) {
	Factory* factory = getInstance();
	ScopeLock lock(factory->_mutex);
	if (factory->_configures.find(name) == factory->_configures.end()) {
		UM_LOG_WARN("No configuration registered for %s", name.c_str());
		return shared_ptr<Configuration>();
	}
	shared_ptr<Configuration> config = factory->_configures[name]->create();
	return config;
}

shared_ptr<Implementation> Factory::create(string name) {
	Factory* factory = getInstance();
	ScopeLock lock(factory->_mutex);
	if (factory->_prototypes.find(name) == factory->_prototypes.end()) {
		UM_LOG_WARN("No prototype registered for %s", name.c_str());
		return shared_ptr<Implementation>();
	}
	shared_ptr<Implementation> implementation = factory->_prototypes[name]->create();
	factory->_implementations.push_back(implementation);
	return implementation;
}

void Factory::registerPrototype(string name, Implementation* prototype, Configuration* config) {
	Factory* factory = getInstance();
	ScopeLock lock(factory->_mutex);
	if (factory->_prototypes.find(name) != factory->_prototypes.end()) {
		UM_LOG_WARN("Overwriting existing prototype for %s", name.c_str());
	}
	factory->_prototypes[name] = prototype;
	factory->_configures[name] = config;
}

/**
 * Iterate created instances from newest to oldest and send suspend request.
 */
void Factory::suspendInstances() {
	Factory* factory = getInstance();
	UMUNDO_LOCK(factory->_mutex);
	vector<weak_ptr<Implementation> >::reverse_iterator implIter = factory->_implementations.rbegin();
	while(implIter != factory->_implementations.rend()) {
		shared_ptr<Implementation> implementation = implIter->lock();
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
	vector<weak_ptr<Implementation> >::iterator implIter = factory->_implementations.begin();
	while(implIter != factory->_implementations.end()) {
		shared_ptr<Implementation> implementation = implIter->lock();
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