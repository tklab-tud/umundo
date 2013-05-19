#ifndef BROADCASTDISCOVERY_H_ZZW33B2N
#define BROADCASTDISCOVERY_H_ZZW33B2N

#include "umundo/common/Common.h"
#include "umundo/thread/Thread.h"
#include "umundo/discovery/Discovery.h"


namespace umundo {

class NodeQuery;

/**
 * Concrete discovery implementor for Broadcast (bridge pattern).
 *
 * This class is a concrete implementor (in the bridge pattern sense) for the Discovery subsystem.
 */
class DLLEXPORT BroadcastNodeDiscovery : public DiscoveryImpl, public Thread {
public:
	virtual ~BroadcastNodeDiscovery();
	static shared_ptr<BroadcastNodeDiscovery> getInstance();  ///< Return the singleton instance.

	shared_ptr<Implementation> create();
	void init(shared_ptr<Configuration>);
	void suspend();
	void resume();

	void add(NodeImpl* node);
	void remove(NodeImpl* node);

	void browse(shared_ptr<NodeQuery> discovery);
	void unbrowse(shared_ptr<NodeQuery> discovery);

	void run();

protected:
	BroadcastNodeDiscovery();
	static shared_ptr<BroadcastNodeDiscovery> _instance;  ///< The singleton instance.

	friend class Factory;
};

}


#endif /* end of include guard: BROADCASTDISCOVERY_H_ZZW33B2N */
