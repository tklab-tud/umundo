/**
 *  @file
 *  @brief      Facade for finding Nodes in the network.
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

#ifndef DISCOVERY_H_PWR3M1QA
#define DISCOVERY_H_PWR3M1QA

#include "umundo/common/Implementation.h"
#include "umundo/connection/Node.h"

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

	virtual void add(NodeImpl* node) = 0;
	virtual void remove(NodeImpl* node) = 0;

	virtual void browse(shared_ptr<NodeQuery> discovery) = 0;
	virtual void unbrowse(shared_ptr<NodeQuery> discovery) = 0;

};

/**
 * Abstraction of the discovery subsystem (bridge pattern).
 *
 * Concrete implementors of this class provide advertising and discovery of remote and
 * local nodes within a domain.
 *
 * - Enable remote Discovery implementors to find locally added Nodes as NodeStub%s.
 * - Allow the application to browse for Node%s via NodeQuery%s.
 *
 * \note You are not expected to add your Node%s yourself, their facade class is already
 * adding the node to the discovery instance. @see Node::create()
 *
 * \sa DiscoveryImpl, Factory, NodeQuery, NodeStub
 */
class DLLEXPORT Discovery {
public:
	/**
	 * Create a new discovery subsystem.
	 *
	 * This will call the Factory to instantiate a new concrete implementor from its registered prototype.
	 *
	 */
	Discovery(); ///< Create a new discovery subsystem
	~Discovery();

	/** @name Node management */
	//@{
	static void add(NodeImpl* node);    /**< Add a Node to multicast domain discovery.
		@param node Node to be added.
	*/
	static void add(Node* node) {
		add(boost::static_pointer_cast<NodeImpl>(node->getImpl()).get());
	}

	static void remove(NodeImpl* node); /**< Remove a Node from multicast domain discovery.
		@param node Previously added Node to be removed.
	*/
	static void remove(Node* node) {
		remove(boost::static_pointer_cast<NodeImpl>(node->getImpl()).get());
	}

	//@}

	/** @name Query for nodes */
	//@{
	static void browse(shared_ptr<NodeQuery> query); ///< Add a query for NodeStub%s
	static void unbrowse(shared_ptr<NodeQuery> query); ///< Remove a query for NodeStub%s
	//@}

protected:
	shared_ptr<DiscoveryImpl> _impl; ///< The concrete implementor instance.
};

}

#endif /* end of include guard: DISCOVERY_H_PWR3M1QA */
