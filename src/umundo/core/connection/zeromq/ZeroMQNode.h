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
	void added(EndPoint);    ///< A node was added, connect to its router socket and list our publishers.
	void removed(EndPoint);  ///< A node was removed, notify local subscribers and clean up.
	void changed(EndPoint);  ///< Never happens.
	//@}


	static uint16_t bindToFreePort(void* socket, const std::string& transport, const std::string& address);
	static void* getZeroMQContext();

protected:
	class NodeConnection {
	public:
		NodeConnection();
		NodeConnection(const std::string& _address, const std::string& thisUUID);
		virtual ~NodeConnection();

		bool connectedTo; ///< are we connected to this node?
		bool connectedFrom; ///< are we connected from this node?

		void* socket; ///< when connect to, the connection to the remote node socket
		std::string socketId; ///< the nodes uuid
		std::string address; ///< when connect to, the address of the remote node socket
		uint64_t startedAt; ///< when connect to, a timestamp when we initially tried to connect
		NodeStub node; /// always a representation about the remote node
		int refCount; ///< when connect to, how many times this address was added as an endpoint
		bool isConfirmed; ///< when connect to, whether we received any node info reply

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
	std::map<std::string, SharedPtr<NodeConnection> > _connFrom; ///< other node uuids connected to us have seen
	std::map<std::string, SharedPtr<NodeConnection> > _connTo; ///< actual connection we maintain to other nodes keys are address and uuid
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
	void sendSubRemoved(const char* nodeUUID, const umundo::Subscriber& sub, const umundo::PublisherStub& pub);
	void sendSubAdded(const char* nodeUUID, const umundo::Subscriber& sub, const umundo::PublisherStub& pub);
	void confirmSub(const std::string& subUUID);
	void processRemotePubAdded(char* nodeUUID, PublisherStubImpl* pub);
	void processRemotePubRemoved(char* nodeUUID, PublisherStubImpl* pub);
	//@}

	/** @name Read / Write to raw byte arrays */
	//@{
	char* writePubInfo(char* buffer, const PublisherStub& pub);
	char* readPubInfo(char* buffer, size_t available, PublisherStubImpl* pub);
	char* writeSubInfo(char* buffer, const Subscriber& sub);
	char* readSubInfo(char* buffer, size_t available, SubscriberStubImpl* sub);
	char* writeVersionAndType(char* buffer, Message::Type type);
	char* readVersionAndType(char* buffer, uint16_t& version, umundo::Message::Type& type);
	char* writeString(char* buffer, const char* content, size_t length);
	char* readString(char* buffer, char*& content, size_t maxLength);
	char* writeUInt16(char* buffer, uint16_t value);
	char* readUInt16(char* buffer, uint16_t& value);
	//@}

	void disconnectRemoteNode(NodeStub& stub);
	void processNodeComm();
	void processPubComm();
	void processOpComm();
	void processClientComm(SharedPtr<NodeConnection> client);
	void processNodeInfo(char* recvBuffer, size_t msgSize);
	void writeNodeInfo(zmq_msg_t* msg, Message::Type type);

	void processConnectedFrom(const std::string& uuid);
	void processConnectedTo(const std::string& uuid, SharedPtr<NodeConnection> client);

	void broadCastNodeInfo(uint64_t now);
	void removeStaleNodes(uint64_t now);

	void replyWithDebugInfo(const std::string uuid);
	StatBucket<double> accumulateIntoBucket();
private:
	static void* _zmqContext; ///< global 0MQ context.


	friend class Factory;
};

}

#endif /* end of include guard: ZEROMQDISPATCHER_H_XFMTSVLV */
