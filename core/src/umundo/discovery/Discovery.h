/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
 */

#ifndef DISCOVERY_H_PWR3M1QA
#define DISCOVERY_H_PWR3M1QA

#include "umundo/common/Implementation.h"

namespace umundo {

class NodeImpl;
class NodeQuery;
class Node;

class DLLEXPORT DiscoveryConfig : public Configuration {
	// at the moment, there is nothing we need to configure
};

/**
 * Discovery implementor basis class (bridge pattern).
 * \see Discovery
 */
class DLLEXPORT DiscoveryImpl : public Implementation {
public:

	virtual void add(shared_ptr<NodeImpl> node) = 0;
	virtual void remove(shared_ptr<NodeImpl> node) = 0;

	virtual void browse(shared_ptr<NodeQuery> discovery) = 0;
	virtual void unbrowse(shared_ptr<NodeQuery> discovery) = 0;

};

/**
 * Abstraction of the discovery subsystem (bridge pattern).
 *
 * Concrete implementors of this class provide advertising and discovery of remote and local nodes within a domain.
 *
 * - Enable remote Discovery implementors to find locally added Nodes as NodeStub%s.
 * - Allow the application to browse for Node%s via NodeQuery%s.
 *
 * \see DiscoveryImpl defines the abstract base class for concrete implementors.
 */
class DLLEXPORT Discovery {
public:
	/**
	 * Create a new discovery subsystem.
	 *
	 * This will call the Factory to instantiate a new concrete implementor from its registered prototype.
	 */
	Discovery(); ///< Create a new discovery subsystem
	~Discovery();

	/** @name Management of local nodes */
	//@{
	static void add(Node* node);    ///< Add a Node to multicast domain discovery.
	static void remove(Node* node); ///< Remove a Node from multicast domain disc.
	//@}

	/** @name Query for nodes */
	//@{
	static void browse(shared_ptr<NodeQuery> discovery);
	static void unbrowse(shared_ptr<NodeQuery> discovery);
	//@}

protected:
	shared_ptr<DiscoveryImpl> _impl; ///< The concrete implementor instance.
};

}

#endif /* end of include guard: DISCOVERY_H_PWR3M1QA */
