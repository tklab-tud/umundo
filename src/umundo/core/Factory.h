/**
 *  @file
 *  @brief      Instantiates implementors at run-time.
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

#ifndef FACTORY_H_BRZEE6H
#define FACTORY_H_BRZEE6H

#include "umundo/core/Common.h"
#include "umundo/core/thread/Thread.h"

namespace umundo {

class Implementation;
class Options;

/**
 * Creates instances of implementations for subsystems at runtime (factory pattern).
 *
 * This class realizes the Factory pattern by instantiating objects form prototypes. If you want to implement a specific subsystem
 * yourself, just inherit its base-class and register a prototype at the factory.
 *
 * \see PublisherImpl, SubscriberImpl, DiscoveryImpl, NodeImpl
 */
class UMUNDO_API Factory {
public:
	static Factory* getInstance();
	static SharedPtr<Implementation> create(const std::string&);

	static void suspendInstances(); ///< Suspend all instances for device sleep
	static void resumeInstances(); ///< Resume all instances from device sleep

	static void registerPrototype(const std::string&, Implementation*);

protected:
	Factory();

private:
	std::map<std::string, Implementation*> _prototypes;
	std::vector<WeakPtr<Implementation> > _implementations;
	RMutex _mutex;
	static Factory* _instance;

};

}

#endif /* end of include guard: FACTORY_H_BRZEE6H */
