/**
 *  @file
 *  @brief      Node implementation with 0MQ.
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

#ifndef ZEROMQDISPATCHER_H_XFMTSVLV
#define ZEROMQDISPATCHER_H_XFMTSVLV

#include <zmq.h>

#include "umundo/core/Common.h"
#include "umundo/core/thread/Thread.h"
#include "umundo/core/ResultSet.h"
#include "umundo/core/connection/Node.h"
#include "umundo/core/Message.h"

/// Send uuid as first message in envelope
#define ZMQ_SEND_IDENTITY(msg, uuid, socket) \
zmq_msg_init(&msg) && UM_LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, uuid.length()) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), uuid.c_str(), uuid.length()); \
zmq_sendmsg(socket, &msg, ZMQ_SNDMORE) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno)); \
zmq_msg_close(&msg) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

/// Initialize a zeromq message with a given size
#define ZMQ_PREPARE(msg, size) \
zmq_msg_init(&msg) && UM_LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno));

/// Initialize a zeromq message with a given size and memcpy data
#define ZMQ_PREPARE_DATA(msg, data, size) \
zmq_msg_init(&msg) && UM_LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), data, size);

/// Initialize a zeromq message with a given null-terminated string
#define ZMQ_PREPARE_STRING(msg, data, size) \
zmq_msg_init(&msg) && UM_LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size + 1) && UM_LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), data, size + 1);

/// send a message to break a zmq_poll and alter some socket
#define ZMQ_INTERNAL_SEND(command, parameter) \
zmq_msg_t socketOp; \
ZMQ_PREPARE_STRING(socketOp, command, strlen(command)); \
zmq_sendmsg(_writeOpSocket, &socketOp, ZMQ_SNDMORE) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno)); \
zmq_msg_close(&socketOp) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno)); \
zmq_msg_t endPointOp; \
ZMQ_PREPARE_STRING(endPointOp, parameter, strlen(parameter)); \
zmq_sendmsg(_writeOpSocket, &endPointOp, 0) >= 0 || UM_LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno)); \
zmq_msg_close(&endPointOp) && UM_LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

namespace umundo {

class PublisherStub;
class ZeroMQPublisher;
class ZeroMQSubscriber;
class NodeQuery;

/**
 * Concrete node implementor for 0MQ (bridge pattern).
 */
class UMUNDO_API ZeroMQNode : public Thread, public NodeImpl, public EnableSharedFromThis<ZeroMQNode> {
public:

	virtual ~ZeroMQNode();

	/** @name Implementor */
	//@{
	SharedPtr<Implementation> create();
	void init(const Options*);
	void suspend();
	void resume();
	//@}

	/** @name Publish / Subscriber Maintenance */
	//@{
	void addSubscriber(Subscriber&);
	void removeSubscriber(Subscriber&);
	void addPublisher(Publisher&);
	void removePublisher(Publisher&);
	std::map<std::string, NodeStub> connectedFrom();
	std::map<std::string, NodeStub> connectedTo();
	//@}

	/** @name Callbacks from Discovery */
	//@{
	void added(ENDPOINT_RS_TYPE);    ///< A node was added, connect to its router socket and list our publishers.
	void removed(ENDPOINT_RS_TYPE);  ///< A node was removed, notify local subscribers and clean up.
	void changed(ENDPOINT_RS_TYPE, uint64_t what = 0);  ///< Never happens.
	//@}


	static uint16_t bindToFreePort(void* socket, const std::string& transport, const std::string& address);
	static void* getZeroMQContext();

protected:

	/**
	 * Message types for internal op socket
	 */
	enum Type {
		UM_DISCONNECT         = 0x0008, // node was removed
	};

	class NodeConnection {
	public:
		NodeConnection(const std::string& _socketId);
		NodeConnection(const std::string& _address, const std::string& _socketId);
		virtual ~NodeConnection();

		void* socket; ///< when connect to, the connection to the remote node socket
		std::string address; ///< when connect to, the address of the remote node socket
		std::string socketId; ///< the containing nodes uuid as a sockt identifier
		uint64_t startedAt; ///< when connect to, a timestamp when we initially tried to connect
		NodeStub node; /// a representation about the remote node
		bool isConfirmed; ///< Whether we connected our subscribers to the remote nodes publishers

		int connect();
		int disconnect();
	};

	class Subscription {
	public:
		Subscription();

		bool isZMQConfirmed; ///< Whether we have seen this subscriber on the XPUB socket
		std::string nodeUUID; ///< the remote node uuid
		std::string address; ///< the remote address from getpeer*
		SubscriberStub subStub; ///< local representation of remote subscriber
		std::map<std::string, Publisher> pending; ///< Subscription pending
		std::map<std::string, Publisher> confirmed; ///< Subscription confirmed
		uint64_t startedAt; ///< Timestamp when we noticed this subscription attempt
	};

	template<class T>
	class StatBucket {
	public:
		StatBucket() :
			timeStamp(Thread::getTimeStampMs()),
			nrMetaMsgRcvd(0),
			sizeMetaMsgRcvd(0),
			nrMetaMsgSent(0),
			sizeMetaMsgSent(0) {
		};
		uint64_t timeStamp;
		std::map<std::string, T> nrChannelMsg; ///< number of message received per channel
		std::map<std::string, T> sizeChannelMsg; ///< accumulate size of messages
		T nrMetaMsgRcvd;
		T sizeMetaMsgRcvd;
		T nrMetaMsgSent;
		T sizeMetaMsgSent;
	};

	std::list<StatBucket<size_t> > _buckets;

	ZeroMQNode();

	std::map<std::string, std::string> _options;

	uint16_t _pubPort; ///< tcp port where we maintain the node-global publisher
	std::map<std::string, NodeStub > _connFrom; ///< other node stubs per uuids connected to us we have seen
	std::map<std::string, SharedPtr<NodeConnection> > _connTo; ///< actual connection we maintain to other nodes, keys are both: address and uuid
	std::map<std::string, SharedPtr<NodeConnection> > _connPending;

	std::map<std::string, Subscription> _subscriptions;

	RMutex _mutex;
	uint64_t _lastNodeInfoBroadCast;
	uint64_t _lastDeadNodeRemoval;
	bool _allowLocalConns;

	bool _dirtySockets;
	zmq_pollitem_t _stdSockets[4]; // standard sockets to poll for this node
	size_t _nrStdSockets;
	zmq_pollitem_t* _sockets;
	size_t _nrSockets;
	std::list<std::pair<uint32_t, std::string> > _nodeSockets;

	void updateSockets();

	void* _nodeSocket; ///< global node socket for off-band communication
	void* _pubSocket; ///< node-global publisher to wrap added publishers
	void* _writeOpSocket; ///< node-internal communication pair to guard zeromq operations from threads
	void* _readOpSocket; ///< node-internal communication pair to guard zeromq operations from threads
	void* _subSocket; ///< umundo internal socket to receive publications from publishers
	void* _monitorSocket;

	void run(); ///< see Thread

	/** @name Remote publisher / subscriber maintenance */
	//@{
	void sendUnsubscribeFromPublisher(const std::string& nodeUUID, const umundo::Subscriber& sub, const umundo::PublisherStub& pub);
	void sendSubscribeToPublisher(const std::string& nodeUUID, const umundo::Subscriber& sub, const umundo::PublisherStub& pub);
	void confirmSubscription(const std::string& subUUID);
	void receivedRemotePubAdded(SharedPtr<NodeConnection> client, SharedPtr<PublisherStubImpl> pub);
	void receivedRemotePubRemoved(SharedPtr<NodeConnection> client, SharedPtr<PublisherStubImpl> pub);
	//@}

	/** @name Read / Write to raw byte arrays - convenience methods */
	//@{
	char* write(char* buffer, const PublisherStub& pub);
	char* write(char* buffer, const Subscriber& sub);
	const char* read(const char* buffer, PublisherStubImpl* pub, size_t available);
	const char* read(const char* buffer, SubscriberStubImpl* sub, size_t available);
	char* writeVersionAndType(char* buffer, uint16_t type);
	const char* readVersionAndType(const char* buffer, uint16_t& version, uint16_t& type);
	//@}

	void unsubscribeFromRemoteNode(SharedPtr<NodeConnection> connection);
	void receivedFromNodeSocket();
	void receivedFromPubSocket();
	void receivedInternalOp();
	void receivedFromClientNode(SharedPtr<NodeConnection> client);

	void remoteNodeConnect(const std::string& address);
	void remoteNodeDisconnect(const std::string& address);
	void remoteNodeConfirm(const std::string& uuid, SharedPtr<NodeConnection> client, const std::list<SharedPtr<PublisherStubImpl> >& publishers);

	void writeNodeInfo(zmq_msg_t* msg, Message::Type type);

//	void broadCastNodeInfo(uint64_t now);
//	void removeStaleNodes(uint64_t now);

	void replyWithDebugInfo(const std::string uuid);
	StatBucket<double> accumulateIntoBucket();

	std::map<std::string, std::set<EndPoint> > _endPoints; ///< 0mq addresses to endpoints added
private:
	static void* _zmqContext; ///< global 0MQ context.


	friend class Factory;
};

}

#endif /* end of include guard: ZEROMQDISPATCHER_H_XFMTSVLV */
