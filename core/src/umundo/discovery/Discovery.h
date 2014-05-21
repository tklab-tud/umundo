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

/**
 * Discovery implementor basis class (bridge pattern).
 * \see Discovery
 */
class UMUNDO_API DiscoveryImpl : public Implementation {
public:
	virtual ~DiscoveryImpl() {}

	virtual void advertise(const EndPoint& node) = 0;
	virtual void add(Node& node) = 0;
	virtual void unadvertise(const EndPoint& node) = 0;
	virtual void remove(Node& node) = 0;

	virtual void browse(ResultSet<EndPoint>* query) = 0;
	virtual void unbrowse(ResultSet<EndPoint>* query) = 0;

	virtual std::vector<EndPoint> list() = 0;
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
class UMUNDO_API Discovery {
public:

	enum DiscoveryType {
	    MDNS,
	    BROADCAST
	};

	/**
	 * Create a new discovery subsystem.
	 *
	 * This will call the Factory to instantiate a new concrete implementor from its registered prototype.
	 *
	 */
	Discovery(DiscoveryType, Options* = NULL); ///< Get the discovery subsystem with given type
	Discovery(DiscoveryType, const std::string domain); ///< Convenience constructor to pass a domain
	Discovery(SharedPtr<DiscoveryImpl> const impl) : _impl(impl) { }
	Discovery(const Discovery& other) : _impl(other._impl) { }
	virtual ~Discovery();
	operator bool() const {
		return _impl.get();
	}
	bool operator< (const Discovery& other) const {
		return _impl < other._impl;
	}
	bool operator==(const Discovery& other) const {
		return _impl == other._impl;
	}
	bool operator!=(const Discovery& other) const {
		return _impl != other._impl;
	}

	Discovery& operator=(const Discovery& other) {
		_impl = other._impl;
		return *this;
	} // operator=

	/** @name Node management */
	//@{

	/** Add a Node to multicast domain discovery.
		@param node Node to be added.
	 */
	void advertise(const EndPoint& endpoint) {
		return _impl->advertise(endpoint);
	}

	void add(Node& node) {
		return _impl->add(node);
	}

	/** Remove a Node from multicast domain discovery.
		@param node Previously added Node to be removed.
	 */
	void unadvertise(const EndPoint& endpoint) {
		return _impl->unadvertise(endpoint);
	}

	void remove(Node& node) {
		return _impl->remove(node);
	}

	//@}

	/** @name Query for nodes */
	//@{

	/// Add a listener for EndPoint%s
	void browse(ResultSet<EndPoint>* query) {
		return _impl->browse(query);
	}

	/// Remove a listener for EndPoint%s
	void unbrowse(ResultSet<EndPoint>* query) {
		return _impl->unbrowse(query);
	}

	/// List all currently known EndPoint%s
	std::vector<EndPoint> list() {
		return _impl->list();
	}
	//@}

protected:
	SharedPtr<DiscoveryImpl> _impl; ///< The concrete implementor instance.
};


class UMUNDO_API MDNSDiscoveryOptions : public Options {
public:
	enum Protocol {
	    TCP, UDP
	};

	MDNSDiscoveryOptions() {
		options["mdns.domain"] = "local.";
		options["mdns.protocol"] = "tcp";
		options["mdns.serviceType"] = "umundo";
	}

	std::string getType() {
		return "mdns";
	}

	void setDomain(const std::string& domain) {
		options["mdns.domain"] = domain;
	}

	void setProtocol(Protocol protocol) {
		switch (protocol) {
		case UDP:
			options["mdns.protocol"] = "udp";
			break;
		case TCP:
			options["mdns.protocol"] = "tcp";
			break;

		default:
			break;
		}
	}

	void setServiceType(const std::string& serviceType) {
		options["mdns.serviceType"] = serviceType;
	}

};

}

#endif /* end of include guard: DISCOVERY_H_PWR3M1QA */
