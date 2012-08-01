/**
 *  @file
 *  @brief      A query representation for NodeStubs.
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

#ifndef NODEQUERY_H_3YGLQKC0
#define NODEQUERY_H_3YGLQKC0

#include <boost/enable_shared_from_this.hpp>

#include "umundo/common/Common.h"

#include "umundo/thread/Thread.h"
#include "umundo/common/ResultSet.h"

namespace umundo {

class NodeStub;

/**
 * Representation of a query for discovery of nodes.
 *
 * Support for the additional query criteria of this class is rather limited in the concrete
 * discovery implementors. At the moment, this is just an extension point to allow more refined
 * queries if we need such a feature.
 */
class DLLEXPORT NodeQuery : public boost::enable_shared_from_this<NodeQuery> {
public:
	NodeQuery(string domain, ResultSet<NodeStub>*);
	virtual ~NodeQuery();

	virtual void found(shared_ptr<NodeStub>);
	virtual void removed(shared_ptr<NodeStub>);

	virtual const string& getDomain();
	virtual void setTransport(string);
	virtual const string& getTransport();

	map<string, shared_ptr<NodeStub> >& getNodes() {
		return _nodes;
	}

	virtual void notifyImmediately(bool notifyImmediately);
	virtual void notifyResultSet();

protected:
	bool _notifyImmediately;
	string _domain;
	string _transport;
	map<string, shared_ptr<NodeStub> > _nodes;
	ResultSet<NodeStub>* _listener;

	Mutex _mutex;

	set<shared_ptr<NodeStub> > _pendingRemovals;
	set<shared_ptr<NodeStub> > _pendingFinds;

	friend class DiscoveryImpl;
};
}

#endif /* end of include guard: NODEQUERY_H_3YGLQKC0 */
