#include <zmq.h>
#include "umundo/core.h"

#include <boost/enable_shared_from_this.hpp>

/// Initialize a zeromq message with a given size
#define ZMQ_PREPARE(msg, size) \
zmq_msg_init(&msg) && LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno));

/// Initialize a zeromq message with a given size and memcpy data
#define ZMQ_PREPARE_DATA(msg, data, size) \
zmq_msg_init(&msg) && LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size) && LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), data, size);

/// Initialize a zeromq message with a given null-terminated string
#define ZMQ_PREPARE_STRING(msg, data, size) \
zmq_msg_init(&msg) && LOG_WARN("zmq_msg_init: %s", zmq_strerror(errno)); \
zmq_msg_init_size (&msg, size + 1) && LOG_WARN("zmq_msg_init_size: %s",zmq_strerror(errno)); \
memcpy(zmq_msg_data(&msg), data, size + 1);

/// Size of a publisher info on wire
#define PUB_INFO_SIZE(pub) \
pub.getChannelName().length() + 1 + pub.getUUID().length() + 1 + 2

/// Size of a subcriber info on wire
#define SUB_INFO_SIZE(sub) \
sub.getChannelName().length() + 1 + sub.getUUID().length() + 1

int receptions = 0;
void *context = NULL;

class ZeroMQPublisher : public umundo::PublisherImpl, public boost::enable_shared_from_this<ZeroMQPublisher> {
public:
  void* _pubSocket;
  umundo::Mutex _mutex;
  umundo::Monitor _pubLock;
  boost::shared_ptr<umundo::PublisherConfig> _config;

  std::map<std::string, std::list<std::pair<uint64_t, umundo::Message*> > > _queuedMessages;

  ZeroMQPublisher() {}

  void init(boost::shared_ptr<umundo::Configuration> config) {
    umundo::ScopeLock lock(_mutex);

    _transport = "tcp";

    (_pubSocket = zmq_socket(context, ZMQ_PUB)) || LOG_WARN("zmq_socket: %s",zmq_strerror(errno));
    
    int hwm = 100000;
    std::string pubId("um.pub.intern." + _uuid);

    zmq_setsockopt(_pubSocket, ZMQ_IDENTITY, pubId.c_str(), pubId.length()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_setsockopt(_pubSocket, ZMQ_SNDHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_bind(_pubSocket, std::string("inproc://" + pubId).c_str());
    
    LOG_INFO("creating internal publisher for %s on %s", _channelName.c_str(), std::string("inproc://" + pubId).c_str());

  }
  
  ~ZeroMQPublisher() {
    LOG_INFO("deleting publisher for %s", _channelName.c_str());

    zmq_close(_pubSocket);
    
    // clean up pending messages
    std::map<std::string, std::list<std::pair<uint64_t, umundo::Message*> > >::iterator queuedMsgSubIter = _queuedMessages.begin();
    while(queuedMsgSubIter != _queuedMessages.end()) {
      std::list<std::pair<uint64_t, umundo::Message*> >::iterator queuedMsgIter = queuedMsgSubIter->second.begin();
      while(queuedMsgIter != queuedMsgSubIter->second.end()) {
        delete (queuedMsgIter->second);
        queuedMsgIter++;
      }
      queuedMsgSubIter++;
    }

  }
  
  boost::shared_ptr<umundo::Implementation> create() {
    return boost::shared_ptr<ZeroMQPublisher>(new ZeroMQPublisher());
  }
  
	void suspend() {
    umundo::ScopeLock lock(_mutex);
    if (_isSuspended)
      return;
    _isSuspended = true;
  };
  
	void resume() {
    umundo::ScopeLock lock(_mutex);
    if (!_isSuspended)
      return;
    _isSuspended = false;
  };
  
  int waitForSubscribers(int count, int timeoutMs) {
    umundo::ScopeLock lock(_mutex);
    uint64_t now = umundo::Thread::getTimeStampMs();
    while (_subUUIDs.size() < (unsigned int)count) {
      _pubLock.wait(_mutex, timeoutMs);
      if (timeoutMs > 0 && umundo::Thread::getTimeStampMs() - timeoutMs > now)
        break;
    }
    /**
     * TODO: we get notified when the subscribers uuid occurs, that 
     * might be before the actual channel is subscribed. I posted
     * to the ZeroMQ ML regarding socket IDs with every subscription 
     * message. Until then, sleep a bit.
     */
    umundo::Thread::sleepMs(20);
    return _subUUIDs.size();
  }

  void addedSubscriber(const std::string remoteId, const std::string subId) {
    umundo::ScopeLock lock(_mutex);
    _subUUIDs.insert(subId);
    
    LOG_ERR("Publisher %s received subscriber %s on node %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(subId).c_str(), SHORT_UUID(remoteId).c_str());
    
    if (_greeter != NULL)
      _greeter->welcome(umundo::Publisher(boost::static_pointer_cast<umundo::PublisherImpl>(shared_from_this())), remoteId, subId);

    if (_queuedMessages.find(subId) != _queuedMessages.end()) {
      LOG_INFO("Subscriber with queued messages joined, sending %d old messages", _queuedMessages[subId].size());
      std::list<std::pair<uint64_t, umundo::Message*> >::iterator msgIter = _queuedMessages[subId].begin();
      while(msgIter != _queuedMessages[subId].end()) {
        send(msgIter->second);
        msgIter++;
      }
      _queuedMessages.erase(subId);
    }
    UMUNDO_SIGNAL(_pubLock);
  }
  
  void removedSubscriber(const std::string remoteId, const std::string subId) {
    umundo::ScopeLock lock(_mutex);
    
    if (_subUUIDs.find(subId) == _subUUIDs.end())
      return;
    _subUUIDs.erase(subId);
    
    LOG_ERR("Publisher %s lost subscriber %s from node %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(subId).c_str(), SHORT_UUID(remoteId).c_str());

    if (_greeter != NULL)
      _greeter->farewell(umundo::Publisher(boost::static_pointer_cast<umundo::PublisherImpl>(shared_from_this())), remoteId, subId);
    UMUNDO_SIGNAL(_pubLock);
  }

  void send(umundo::Message* msg) {
    if (_isSuspended) {
      LOG_WARN("Not sending message on suspended publisher");
      return;
    }
    
    // topic name or explicit subscriber id is first message in envelope
    zmq_msg_t channelEnvlp;
    if (msg->getMeta().find("um.sub") != msg->getMeta().end()) {
      // explicit destination
      if (_subUUIDs.find(msg->getMeta("um.sub")) == _subUUIDs.end() && !msg->isQueued()) {
        LOG_INFO("Subscriber %s is not (yet) connected on %s - queuing message", msg->getMeta("um.sub").c_str(), _channelName.c_str());
        umundo::Message* queuedMsg = new umundo::Message(*msg); // copy message
        queuedMsg->setQueued(true);
        _queuedMessages[msg->getMeta("um.sub")].push_back(std::make_pair(umundo::Thread::getTimeStampMs(), queuedMsg));
        return;
      }
      ZMQ_PREPARE_STRING(channelEnvlp, msg->getMeta("um.sub").c_str(), msg->getMeta("um.sub").size());
    } else {
      // everyone on channel
      ZMQ_PREPARE_STRING(channelEnvlp, _channelName.c_str(), _channelName.size());
    }
    
    // mandatory meta fields
    msg->putMeta("um.pub", _uuid);
    msg->putMeta("um.proc", umundo::procUUID);
    msg->putMeta("um.host", umundo::Host::getHostId());
    
    std::map<std::string, std::string>::const_iterator metaIter = _mandatoryMeta.begin();
    while(metaIter != _mandatoryMeta.end()) {
      if (metaIter->second.length() > 0)
        msg->putMeta(metaIter->first, metaIter->second);
      metaIter++;
    }
    
    zmq_sendmsg(_pubSocket, &channelEnvlp, ZMQ_SNDMORE) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&channelEnvlp) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
    
    // all our meta information
    for (metaIter = msg->getMeta().begin(); metaIter != msg->getMeta().end(); metaIter++) {
      // string key(metaIter->first);
      // string value(metaIter->second);
      // std::cout << key << ": " << value << std::endl;
      
      // string length of key + value + two null bytes as string delimiters
      size_t metaSize = (metaIter->first).length() + (metaIter->second).length() + 2;
      zmq_msg_t metaMsg;
      ZMQ_PREPARE(metaMsg, metaSize);
      
      char* writePtr = (char*)zmq_msg_data(&metaMsg);
      
      memcpy(writePtr, (metaIter->first).data(), (metaIter->first).length());
      // indexes start at zero, so length is the byte after the string
      ((char*)zmq_msg_data(&metaMsg))[(metaIter->first).length()] = '\0';
      assert(strlen((char*)zmq_msg_data(&metaMsg)) == (metaIter->first).length());
      assert(strlen(writePtr) == (metaIter->first).length()); // just to be sure
      
      // increment write pointer
      writePtr += (metaIter->first).length() + 1;
      
      memcpy(writePtr,
             (metaIter->second).data(),
             (metaIter->second).length());
      // first string + null byte + second string
      ((char*)zmq_msg_data(&metaMsg))[(metaIter->first).length() + 1 + (metaIter->second).length()] = '\0';
      assert(strlen(writePtr) == (metaIter->second).length());
      
      zmq_sendmsg(_pubSocket, &metaMsg, ZMQ_SNDMORE) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
      zmq_msg_close(&metaMsg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
    }
    
    // data as the second part of a multipart message
    zmq_msg_t publication;
    ZMQ_PREPARE_DATA(publication, msg->data(), msg->size());
    zmq_sendmsg(_pubSocket, &publication, 0) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&publication) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
  }
};

class ZeroMQSubscriber : public umundo::SubscriberImpl, public umundo::Thread {
public:
  void* _subSocket;
  std::multimap<std::string, std::string> _domainPubs;
  umundo::Mutex _mutex;

  boost::shared_ptr<umundo::SubscriberConfig> _config;

  ZeroMQSubscriber() {}
  
	void init(boost::shared_ptr<umundo::Configuration> config) {
    _config = boost::static_pointer_cast<umundo::SubscriberConfig>(config);
    _subSocket = zmq_socket(context, ZMQ_SUB);
    assert(_channelName.size() > 0);

    int hwm = 100000;
    std::string subId("um.sub." + _uuid);

		zmq_setsockopt(_subSocket, ZMQ_IDENTITY, subId.c_str(), subId.length()) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, subId.c_str(), subId.length())  && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, _channelName.c_str(), _channelName.length())  && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    
    int rcvTimeOut = 30;
    zmq_setsockopt(_subSocket, ZMQ_RCVTIMEO, &rcvTimeOut, sizeof(rcvTimeOut));

    // reconnection intervals
#if 0
    int reconnect_ivl_min = 100;
    int reconnect_ivl_max = 200;
    zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL, &reconnect_ivl_min, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
    zmq_setsockopt (_subSocket, ZMQ_RECONNECT_IVL_MAX, &reconnect_ivl_max, sizeof(int)) && LOG_WARN("zmq_setsockopt: %s",zmq_strerror(errno));
#endif
    
    zmq_bind(_subSocket, std::string("inproc://" + subId).c_str());
    
    if (_receiver)
      start();
  }

  ~ZeroMQSubscriber() {
    LOG_INFO("deleting subscriber for %s", _channelName.c_str());
    stop();
    join();
    zmq_close(_subSocket);
  }

  boost::shared_ptr<umundo::Implementation> create() {
    return boost::shared_ptr<ZeroMQSubscriber>(new ZeroMQSubscriber());
  }
  
	void suspend() {
    umundo::ScopeLock lock(_mutex);
    if (_isSuspended)
      return;
    _isSuspended = true;

  };
  
  void resume() {
    umundo::ScopeLock lock(_mutex);
    if (!_isSuspended)
      return;
    _isSuspended = false;
  };

  void added(umundo::PublisherStub pub) {
    umundo::ScopeLock lock(_mutex);
    if (_pubUUIDs.find(pub.getUUID()) != _pubUUIDs.end())
      return;

    _pubUUIDs.insert(pub.getUUID());
    
    if (_domainPubs.count(pub.getDomain()) == 0) {
      std::cout << "Got a publisher!" << std::endl;
      std::stringstream ss;
      if (pub.isRemote()) {
        // remote node, use network
        ss << pub.getTransport() << "://" << pub.getIP() << ":" << pub.getPort();
      } else if (pub.isInProcess()) {
        // same process, use inproc communication
        ss << "inproc://um.pub." << pub.getDomain();
      } else {
        ss << "ipc://um.pub." << pub.getDomain();
      }
      zmq_connect(_subSocket, ss.str().c_str()) && LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));
    }
    
    _pubUUIDs.insert(pub.getUUID());
    _domainPubs.insert(std::make_pair(pub.getDomain(), pub.getUUID()));
  }

  void removed(umundo::PublisherStub pub) {
    umundo::ScopeLock lock(_mutex);
    if (_pubUUIDs.find(pub.getUUID()) == _pubUUIDs.end())
      return;
    
    assert(_domainPubs.count(pub.getDomain()) != 0);    
    std::multimap<std::string, std::string>::iterator domIter = _domainPubs.find(pub.getDomain());
    while(domIter != _domainPubs.end()) {
      if (domIter->second == pub.getUUID()) {
        _domainPubs.erase(domIter++);
      } else {
        domIter++;
      }
    }
    _pubUUIDs.erase(pub.getUUID());

    if (_domainPubs.count(pub.getDomain()) == 0) {
      std::cout << "Lost a publisher!" << std::endl;
      std::stringstream ss;
      if (pub.isRemote()) {
        // remote node, use network
        ss << pub.getTransport() << "://" << pub.getIP() << ":" << pub.getPort();
      } else if (pub.isInProcess()) {
        // same process, use inproc communication
        ss << "inproc://um.pub." << pub.getDomain();
      } else {
        ss << "ipc://um.pub." << pub.getDomain();
      }
      zmq_disconnect(_subSocket, ss.str().c_str()) && LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));
    }
  }

  void changed(umundo::PublisherStub pub) {
  }

  void setReceiver(umundo::Receiver* receiver) {
    stop();
    join();
    _receiver = receiver;
    if (_receiver != NULL) {
      start();
    }
  }

  void run() {
    zmq_pollitem_t items[1];
    items[0].socket = _subSocket;
    items[0].events = ZMQ_POLLIN | ZMQ_POLLOUT;

    while(isStarted()) {
      int rc = zmq_poll(items, 1, 20);
      if (rc < 0) {
        LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
      }

      if (!isStarted())
        return;
      
      if (items[0].revents & ZMQ_POLLIN && _receiver != NULL) {
        umundo::Message* msg = getNextMsg();
        _receiver->receive(msg);
      }
    }
  }
  
  umundo::Message* getNextMsg() {
    int32_t more;
    size_t more_size = sizeof(more);
    
    umundo::Message* msg = new umundo::Message();
    while (1) {
      // read the whole message
      zmq_msg_t message;
      zmq_msg_init(&message) && LOG_WARN("zmq_msg_init: %s",zmq_strerror(errno));
      
      int rc;
      rc = zmq_recvmsg(_subSocket, &message, ZMQ_DONTWAIT);
      if (rc < 0) {
        LOG_WARN("zmq_recvmsg: %s",zmq_strerror(errno));
        zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
        delete msg;
        return NULL;
      }
      
      size_t msgSize = zmq_msg_size(&message);      
      zmq_getsockopt(_subSocket, ZMQ_RCVMORE, &more, &more_size) && LOG_WARN("zmq_getsockopt: %s",zmq_strerror(errno));
      
      if (more) {
        char* key = (char*)zmq_msg_data(&message);
        char* value = ((char*)zmq_msg_data(&message) + strlen(key) + 1);
        
        // is this the first message with the channelname?
        if (strlen(key) + 1 == msgSize &&
            msg->getMeta().find(key) == msg->getMeta().end()) {
          msg->putMeta("um.channel", key);
        } else {
          if (strlen(key) + strlen(value) + 2 != msgSize) {
            LOG_ERR("Received malformed meta field %d + %d + 2 != %d", strlen(key), strlen(value), msgSize);
            zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
            break;
          } else {
            msg->putMeta(key, value);
          }
        }
        zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
      } else {
        // last message contains actual data
        msg->setData((char*)zmq_msg_data(&message), msgSize);
        zmq_msg_close(&message) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));        
        break; // last message part
      }
    }
    return msg;
  }

  bool hasNextMsg() {
    zmq_pollitem_t items[1];
    items[0].socket = _subSocket;
    items[0].events = ZMQ_POLLIN;
    
    int rc = zmq_poll(items, 1, 0);
    if (rc < 0) {
      LOG_ERR("zmq_poll: %s", zmq_strerror(errno));
      return false;
    }
    if (items[0].revents & ZMQ_POLLIN) {
      return true;
    }
    return false;
  }

};

class ZeroMQNode : public umundo::NodeImpl, public umundo::ResultSet<umundo::NodeStub>, public umundo::Thread {
public:
  void* _pubSocket; // public socket for outgoing external communication
  void* _subSocket; // umundo internal socket to receive publications from publishers
  void* _nodeSocket; // socket for node meta information
  uint16_t _pubPort;

  umundo::Mutex _mutex;
  
  std::multiset<std::string> _subscriptions; ///< subscriptions as we received them from zeromq
  std::multiset<std::string> _confirmedSubscribers; ///< subscriptions as we received them from zeromq and confirmed by node communication
  std::multimap<std::string, std::pair<long, std::pair<umundo::NodeStub, umundo::SubscriberStub> > > _pendingSubscriptions; ///< channel to timestamped pending subscribers we got from node communication
  std::map<std::string, std::map<std::string, umundo::PublisherStub> > _pendingRemotePubs; ///< publishers of yet undiscovered nodes.

  std::map<std::string, umundo::NodeStub> _nodes; ///< subscriber confirmed by node communication and zeromq
  std::map<std::string, void*> _sockets; ///< UUIDs to ZeroMQ Node Sockets.

  boost::shared_ptr<umundo::NodeQuery> _nodeQuery;

  boost::shared_ptr<umundo::Implementation> create() {
  	return boost::shared_ptr<ZeroMQNode>(new ZeroMQNode());
  }

  ZeroMQNode() {
    init(boost::shared_ptr<umundo::Configuration>());
  }
  
  void init(boost::shared_ptr<umundo::Configuration>) {
    _transport = "tcp";
    
    (_pubSocket  = zmq_socket(context, ZMQ_XPUB))    || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
    (_subSocket  = zmq_socket(context, ZMQ_SUB))     || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));
    (_nodeSocket = zmq_socket(context, ZMQ_ROUTER))  || LOG_ERR("zmq_socket: %s", zmq_strerror(errno));

    int hwm = 5000000;
    int vbsSub = 1;

    std::string pubId("um.pub." + _uuid);
    zmq_setsockopt(_pubSocket, ZMQ_IDENTITY, pubId.c_str(), pubId.length()) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
    zmq_setsockopt(_pubSocket, ZMQ_SNDHWM, &hwm, sizeof(hwm))               && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
    zmq_setsockopt(_pubSocket, ZMQ_XPUB_VERBOSE, &vbsSub, sizeof(vbsSub))   && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // receive all subscriptions

    zmq_setsockopt(_subSocket, ZMQ_RCVHWM, &hwm, sizeof(hwm)) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));
    zmq_setsockopt(_subSocket, ZMQ_SUBSCRIBE, "", 0)          && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno)); // subscribe to every internal publisher

    std::string nodeId("um.node." + _uuid);
    zmq_setsockopt(_nodeSocket, ZMQ_IDENTITY, nodeId.c_str(), nodeId.length()) && LOG_WARN("zmq_setsockopt: %s", zmq_strerror(errno));

    _port = bindToFreePort(_nodeSocket, "tcp", "*");
    zmq_bind(_nodeSocket, std::string("inproc://" + nodeId).c_str()) && LOG_WARN("zmq_bind: %s", zmq_strerror(errno));
    zmq_bind(_nodeSocket, std::string("ipc://" + nodeId).c_str())    && LOG_WARN("zmq_bind: %s", zmq_strerror(errno));

    _pubPort = bindToFreePort(_pubSocket, "tcp", "*");
    zmq_bind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
    zmq_bind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
    
    LOG_ERR("Node %s listening on port %d", SHORT_UUID(_uuid).c_str(), _port);
    LOG_ERR("Publisher %s listening on port %d", SHORT_UUID(_uuid).c_str(), _pubPort);
    
    _nodeQuery = boost::shared_ptr<umundo::NodeQuery>(new umundo::NodeQuery(_domain, this));

    umundo::Discovery::add(this);
    umundo::Discovery::browse(_nodeQuery);
    
    Thread::start();
  }
  
  ~ZeroMQNode() {
    umundo::Discovery::unbrowse(_nodeQuery);
    umundo::Discovery::remove(this);

    Thread::stop();
    
    // close connection to all remote publishers
    std::map<std::string, void*>::iterator sockIter;
    for (sockIter = _sockets.begin(); sockIter != _sockets.end(); sockIter++) {
      zmq_close(sockIter->second) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
    }

    join();
    zmq_close(_pubSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
    zmq_close(_subSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
    zmq_close(_nodeSocket) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));
  }

	void suspend() {
    umundo::ScopeLock lock(_mutex);
    if (_isSuspended)
      return;
    _isSuspended = true;
    
    // stop browsing for remote nodes
    umundo::Discovery::unbrowse(_nodeQuery);
    umundo::Discovery::remove(this);

  };
	
  void resume() {
    umundo::ScopeLock lock(_mutex);
    if (!_isSuspended)
      return;
    _isSuspended = false;

    std::map<std::string, umundo::Subscriber>::iterator localSubIter = _subs.begin();
    while(localSubIter != _subs.end()) {
      localSubIter->second.resume();
      localSubIter++;
    }

    umundo::Discovery::add(this);
    umundo::Discovery::browse(_nodeQuery);
  };

  static uint16_t bindToFreePort(void* socket, const std::string& transport, const std::string& address) {
    std::stringstream ss;
    int port = 4242;
    
    ss << transport << "://" << address << ":" << port;
    
    while(zmq_bind(socket, ss.str().c_str()) == -1) {
      switch(errno) {
        case EADDRINUSE:
          port++;
          ss.clear();        // clear error bits
          ss.str(std::string());  // reset string
          ss << transport << "://" << address << ":" << port;
          break;
        default:
          LOG_WARN("zmq_bind at %s: %s", ss.str().c_str(), zmq_strerror(errno));
          Thread::sleepMs(100);
      }
    }
    
    return port;
  }

  void addSubscriber(umundo::Subscriber& sub) {
    umundo::ScopeLock lock(_mutex);
    if (_subs.find(sub.getUUID()) == _subs.end()) {
      _subs[sub.getUUID()] = sub;
      
      std::map<std::string, umundo::NodeStub>::iterator nodeIter = _nodes.begin();
      while (nodeIter != _nodes.end()) {
        void* nodeSocket = _sockets[nodeIter->first];
        assert(nodeSocket != NULL);
        
        std::map<std::string, umundo::PublisherStub> pubs = nodeIter->second.getPublishers();
        std::map<std::string, umundo::PublisherStub>::iterator pubIter = pubs.begin();
        // iterate all remote publishers
        while (pubIter != pubs.end()) {
          if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
            sub.added(pubIter->second);
            sendSubAdded(nodeSocket, sub, pubIter->second, false);
          }
          pubIter++;
        }
        nodeIter++;
      }
    }
  }

  void removeSubscriber(umundo::Subscriber& sub) {
    umundo::ScopeLock lock(_mutex);
    if (_subs.find(sub.getUUID()) != _subs.end()) {
      std::string subId("inproc://um.sub." + sub.getUUID());
      //zmq_disconnect(_pubSocket, subId.c_str()) && LOG_ERR("zmq_connect %s: %s", subId.c_str(), zmq_strerror(errno));
      _subs.erase(sub.getUUID());

      std::map<std::string, umundo::NodeStub>::iterator nodeIter = _nodes.begin();
      while (nodeIter != _nodes.end()) {
        void* nodeSocket = _sockets[nodeIter->first];
        assert(nodeSocket != NULL);
        
        std::map<std::string, umundo::PublisherStub> pubs = nodeIter->second.getPublishers();
        std::map<std::string, umundo::PublisherStub>::iterator pubIter = pubs.begin();
        // iterate all remote publishers
        while (pubIter != pubs.end()) {
          if(pubIter->second.getChannelName().substr(0, sub.getChannelName().size()) == sub.getChannelName()) {
            sendSubRemoved(nodeSocket, sub, pubIter->second, false);
            sub.removed(pubIter->second);
          }
          pubIter++;
        }
        nodeIter++;
      }

    }
  }
  
  void addPublisher(umundo::Publisher& pub) {
    umundo::ScopeLock lock(_mutex);
    if (_pubs.find(pub.getUUID()) == _pubs.end()) {
      std::string internalPubId("inproc://um.pub.intern." + pub.getUUID());
      zmq_connect(_subSocket, internalPubId.c_str()) && LOG_ERR("zmq_connect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
      
//      std::string pubId("um.pub." + pub.getUUID()); // proxy publisher ipc and inproc for subscribers
//      zmq_bind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))
//      zmq_bind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && LOG_WARN("zmq_bind: %s", zmq_strerror(errno))

      _pubs[pub.getUUID()] = pub;
      std::map<std::string, void*>::iterator socketIter = _sockets.begin();
      // notify all other nodes
      while (socketIter != _sockets.end()) {
        sendPubAdded(socketIter->second, pub, false);
        socketIter++;
      }
    }
  }
  
  void removePublisher(umundo::Publisher& pub) {
    umundo::ScopeLock lock(_mutex);
    if (_pubs.find(pub.getUUID()) != _pubs.end()) {
      std::string internalPubId("inproc://um.pub.intern." + pub.getUUID());
      zmq_disconnect(_subSocket, internalPubId.c_str()) && LOG_ERR("zmq_disconnect %s: %s", internalPubId.c_str(), zmq_strerror(errno));
      
//      std::string pubId("um.pub." + pub.getUUID()); // unproxy publisher ipc and inproc for subscribers
//      zmq_unbind(_pubSocket,  std::string("inproc://" + pubId).c_str())  && LOG_WARN("zmq_unbind: %s", zmq_strerror(errno))
//      zmq_unbind(_pubSocket,  std::string("ipc://" + pubId).c_str())     && LOG_WARN("zmq_unbind: %s", zmq_strerror(errno))

      _pubs.erase(pub.getUUID());
      std::map<std::string, void*>::iterator socketIter = _sockets.begin();
      // notify all other nodes
      while (socketIter != _sockets.end()) {
        sendPubRemoved(socketIter->second, pub, false);
        socketIter++;
      }
    }
  }
  
  void added(umundo::NodeStub node) {
    // is this us?
    if (node.getUUID() == _uuid)
      return;
    umundo::ScopeLock lock(_mutex);

    if (_nodes.find(node.getUUID()) != _nodes.end()) {
      LOG_WARN("Not adding already known node");
      return;
    }
      
    // establish connection
    std::stringstream ss;
    if (node.isRemote()) {
      // remote node, use network
      ss << node.getTransport() << "://" << node.getIP() << ":" << node.getPort();
    } else if (node.isInProcess()) {
      // same process, use inproc communication
      ss << "inproc://um.node." << node.getUUID();
    } else {
      // same host, use inter-process communication
      // TODO: Discovery never reports nodes as inProcess
      ss << "ipc://um.node." << node.getUUID();    
    }

		// connect a socket to the remote node port
		void* client;
		(client = zmq_socket(context, ZMQ_DEALER)) || LOG_ERR("zmq_socket: %s",zmq_strerror(errno));
    
		// give the socket an id for zeroMQ routing
		zmq_setsockopt(client, ZMQ_IDENTITY, _uuid.c_str(), _uuid.length()) && LOG_ERR("zmq_setsockopt: %s",zmq_strerror(errno));
		zmq_connect(client, ss.str().c_str()) && LOG_ERR("zmq_connect %s: %s", ss.str().c_str(), zmq_strerror(errno));
    
		// remember node stub and socket
		_nodes[node.getUUID()] = node;
		_sockets[node.getUUID()] = client;

    // send local publishers to remote node
    std::map<std::string, umundo::Publisher>::iterator pubIter;
    int hasMore = _pubs.size() - 1;
    for (pubIter = _pubs.begin(); pubIter != _pubs.end(); pubIter++, hasMore--) {
			// create a publisher added message from current publisher
      sendPubAdded(client, pubIter->second, hasMore);
		}
    LOG_INFO("sending %d publishers to newcomer", _pubs.size());
    
    if (_pendingRemotePubs.find(node.getUUID()) != _pendingRemotePubs.end()) {
      // the other node found us first and already sent its publishers
      std::map<std::string, umundo::PublisherStub>::iterator pubIter = _pendingRemotePubs[node.getUUID()].begin();
      while(pubIter != _pendingRemotePubs[node.getUUID()].end()) {
        confirmPubAdded(node, pubIter->second);
        pubIter++;
      }
      _pendingRemotePubs.erase(node.getUUID());
    }
  }
  
  void removed(umundo::NodeStub node) {
    // is this us?
    if (node.getUUID() == _uuid)
      return;
    umundo::ScopeLock lock(_mutex);

    if (_nodes.find(node.getUUID()) == _nodes.end()) {
      LOG_WARN("Not removing unknown node");
      return;
    }
    
    std::map<std::string, umundo::PublisherStub> pubs = _nodes[node.getUUID()].getPublishers();
    std::map<std::string, umundo::PublisherStub>::iterator pubIter = pubs.begin();
    while (pubIter != pubs.end()) {
      processPubRemoval(node.getUUID(), pubIter->second);
      pubIter++;
    }

    std::map<std::string, umundo::SubscriberStub> subs = _nodes[node.getUUID()].getSubscribers();
    std::map<std::string, umundo::SubscriberStub>::iterator subIter = subs.begin();
    while (subIter != subs.end()) {
      std::map<std::string, umundo::Publisher>::iterator pubIter = _pubs.begin();
      while (pubIter != _pubs.end()) {
        if(pubIter->second.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
          pubIter->second.removedSubscriber(node.getUUID(), subIter->second.getUUID());
        }
        pubIter++;
      }
      subIter++;
    }
    
    if (_sockets.find(node.getUUID()) == _sockets.end()) {
			LOG_WARN("removed: client was never added: %s", SHORT_UUID(node.getUUID()).c_str());
			return;
		}
    zmq_close(_sockets[node.getUUID()]) && LOG_WARN("zmq_close: %s",zmq_strerror(errno));    
		
    _sockets.erase(node.getUUID());               // delete socket
    _nodes.erase(node.getUUID());
    _pendingRemotePubs.erase(node.getUUID());
    assert(_sockets.size() == _nodes.size());

    LOG_INFO("%s removed %s", SHORT_UUID(_uuid).c_str(), SHORT_UUID(node.getUUID()).c_str());
    
  }
  
  void changed(umundo::NodeStub node) {
  }

  void sendPubRemoved(void* socket, const umundo::Publisher& pub, bool hasMore) {
    zmq_msg_t msg;
    int pubInfoSize = PUB_INFO_SIZE(pub) + 2 + 2;
    ZMQ_PREPARE(msg, pubInfoSize);
    
    char* buffer = (char*)zmq_msg_data(&msg);
    *(uint16_t*)(buffer) = htons(umundo::Message::VERSION);  buffer += 2;
    *(uint16_t*)(buffer) = htons(umundo::Message::PUB_REMOVED);  buffer += 2;
    writePubInfo(buffer, pub);
    
    zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
  }
  
  void sendPubAdded(void* socket, const umundo::Publisher& pub, bool hasMore) {
    zmq_msg_t msg;
    int pubInfoSize = PUB_INFO_SIZE(pub) + 2 + 2;
    ZMQ_PREPARE(msg, pubInfoSize);
    
    char* buffer = (char*)zmq_msg_data(&msg);
    *(uint16_t*)(buffer) = htons(umundo::Message::VERSION);  buffer += 2;
    *(uint16_t*)(buffer) = htons(umundo::Message::PUB_ADDED);  buffer += 2;
    buffer = writePubInfo(buffer, pub);
    
    assert(buffer - (char*)zmq_msg_data(&msg) == zmq_msg_size(&msg));
    
    zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
  }
  
  void sendSubAdded(void* socket, const umundo::Subscriber& sub, const umundo::PublisherStub& pub, bool hasMore) {
    zmq_msg_t msg;
    int subInfoSize = PUB_INFO_SIZE(pub) + SUB_INFO_SIZE(sub) + 2 + 2;
    ZMQ_PREPARE(msg, subInfoSize);
    
    char* buffer = (char*)zmq_msg_data(&msg);
    *(uint16_t*)(buffer) = htons(umundo::Message::VERSION);  buffer += 2;
    *(uint16_t*)(buffer) = htons(umundo::Message::SUBSCRIBE); buffer += 2;
    buffer = writePubInfo(buffer, pub);
    buffer = writeSubInfo(buffer, sub);
    
    assert((size_t)(buffer - (char*)zmq_msg_data(&msg)) == zmq_msg_size(&msg));
    
    zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));
  }

  void sendSubRemoved(void* socket, const umundo::Subscriber& sub, const umundo::PublisherStub& pub, bool hasMore) {
    zmq_msg_t msg;
    int subInfoSize = SUB_INFO_SIZE(sub) + 2 + 2;
    ZMQ_PREPARE(msg, subInfoSize);
    
    char* buffer = (char*)zmq_msg_data(&msg);
    *(uint16_t*)(buffer) = htons(umundo::Message::VERSION);  buffer += 2;
    *(uint16_t*)(buffer) = htons(umundo::Message::UNSUBSCRIBE); buffer += 2;
    buffer = writeSubInfo(buffer, sub);
    
    assert((size_t)(buffer - (char*)zmq_msg_data(&msg)) == zmq_msg_size(&msg));
    
    zmq_sendmsg(socket, &msg, (hasMore ? ZMQ_SNDMORE : 0)) >= 0 || LOG_WARN("zmq_sendmsg: %s",zmq_strerror(errno));
    zmq_msg_close(&msg) && LOG_WARN("zmq_msg_close: %s",zmq_strerror(errno));

  }
  
  void run() {
    char* remoteUUID = NULL;

    int more;
    size_t more_size = sizeof (more);

    //  Initialize poll set
    zmq_pollitem_t items [] = {
      { _pubSocket,  0, ZMQ_POLLIN, 0 }, // read subscriptions
      { _subSocket,  0, ZMQ_POLLIN, 0 }, // publication requests
      { _nodeSocket, 0, ZMQ_POLLIN, 0 }, // node meta communication
    };

    while(isStarted()) {
        zmq_msg_t message;
        
        zmq_poll (items, 3, 20);
        if (!isStarted())
          return;
        
        if (items [0].revents & ZMQ_POLLIN) {
          /**
           * someone subscribed, process here to avoid
           * XPUB socket and thread at publisher
           */
          umundo::ScopeLock lock(_mutex);
          while (1) {
            //  Process all parts of the message
            zmq_msg_init (&message);
            zmq_msg_recv (&message, _pubSocket, 0);
            
            char* data = (char*)zmq_msg_data(&message);
            bool subscription = (data[0] == 0x1);
            std::string subChannel(data+1, zmq_msg_size(&message) - 1);

            if (subscription) {
              LOG_ERR("Got 0MQ subscription on %s", subChannel.c_str());
              _subscriptions.insert(subChannel);
              processZMQCommSubscriptions(subChannel);
            } else {
              LOG_ERR("Got 0MQ unsubscription on %s", subChannel.c_str());
              _subscriptions.erase(subChannel);
              //processZMQCommUnsubscriptions("", subChannel, umundo::SubscriberStub());
            }
                        
            zmq_getsockopt (_pubSocket, ZMQ_RCVMORE, &more, &more_size);
            zmq_msg_close (&message);
            assert(!more); // subscriptions are not multipart
            if (!more)
              break;      //  Last message part
          }
        }

        if (items [1].revents & ZMQ_POLLIN) {
          /**
           * someone internal publisher wants something published
           */
          while (1) {
            //  Process all parts of the message
            zmq_msg_init (&message);
            zmq_msg_recv (&message, _subSocket, 0);
            zmq_getsockopt (_subSocket, ZMQ_RCVMORE, &more, &more_size);
            zmq_msg_send (&message, _pubSocket, more? ZMQ_SNDMORE: 0);
            zmq_msg_close (&message);
            if (!more)
              break;      //  Last message part
          }
        }
        
        if (items [2].revents & ZMQ_POLLIN) {
          /**
           * received some node communication
           */
          umundo::ScopeLock lock(_mutex);

          while (1) {
            //  Process all parts of the message
            zmq_msg_init (&message);
            zmq_msg_recv (&message, _nodeSocket, 0);
            int msgSize = zmq_msg_size(&message);

            zmq_getsockopt (_nodeSocket, ZMQ_RCVMORE, &more, &more_size);

            if (msgSize == 36 && remoteUUID == NULL) {
              // remote id from envelope
              remoteUUID = (char*)malloc(37);
              memcpy(remoteUUID, zmq_msg_data(&message), 36);
              remoteUUID[36] = 0;
            } else if (msgSize >= 2 && remoteUUID != NULL) {
              char* buffer = (char*)zmq_msg_data(&message);
              uint16_t version = ntohs(*(short*)(buffer));   buffer += 2;
              uint16_t type    = ntohs(*(short*)(buffer));   buffer += 2;

              if (version != umundo::Message::VERSION) {
                LOG_ERR("Received message from wrong or unversioned umundo - ignoring");
                zmq_msg_close (&message);
                break;
              }

              switch (type) {
                case umundo::Message::DATA:
                  break;
                case umundo::Message::PUB_ADDED:
                case umundo::Message::PUB_REMOVED: {
                  // notification of publishers is the first thing a remote node does once it discovers us

                  uint16_t port = 0;
                  char* channel;
                  char* pubUUID;
                  buffer = readPubInfo(buffer, port, channel, pubUUID);
                  
                  if (buffer - (char*)zmq_msg_data(&message) != msgSize) {
                    LOG_ERR("Malformed PUB_ADDED|PUB_REMOVED message received - ignoring");
                    break;
                  }

                  if (type == umundo::Message::PUB_ADDED) {
                    umundo::PublisherStub pubStub(boost::shared_ptr<umundo::PublisherStubImpl>(new umundo::PublisherStubImpl()));
                    pubStub.getImpl()->setChannelName(channel);
                    pubStub.getImpl()->setPort(port);
                    pubStub.getImpl()->setUUID(pubUUID);
                    pubStub.getImpl()->setDomain(remoteUUID);
                    processNodeCommPubAdded(remoteUUID, pubStub);
                  } else {
                    if (_nodes.find(remoteUUID) == _nodes.end()) {
                      LOG_ERR("Unknown node removed publisher - ignoring");
                      break;
                    }
                    if (!_nodes[remoteUUID].getPublisher(pubUUID)) {
                      LOG_ERR("Unknown node sent unsubscription - ignoring");
                      break;
                    }
                    processPubRemoval(remoteUUID, _nodes[remoteUUID].getPublisher(pubUUID));
                  }
                  break;
                }
                  
                case umundo::Message::SUBSCRIBE:
                case umundo::Message::UNSUBSCRIBE: {
                  // a remote node tells us of a subscription of one of its subscribers
                  
                  if (_nodes.find(remoteUUID) == _nodes.end()) {
                    LOG_ERR("Got a subscription from an unknown node - this should never happen!");
                    break;
                  }
                  umundo::NodeStub nodeStub = _nodes[remoteUUID];
                  
                  uint16_t port;
                  char* pubChannel;
                  char* pubUUID;
                  char* subChannel;
                  char* subUUID;
                  buffer = readPubInfo(buffer + 2, port, pubChannel, pubUUID);
                  buffer = readSubInfo(buffer, subChannel, subUUID);
                  
                  if (buffer - (char*)zmq_msg_data(&message) != msgSize ||
                      strlen(pubUUID) != 36 ||
                      strlen(subUUID) != 36)
                  {
                    LOG_ERR("Malformed SUBSCRIBE|UNSUBSCRIBE message received - ignoring");
                    break;
                  }
                  
                  if (type == umundo::Message::SUBSCRIBE) {
                    if (!nodeStub.getSubscriber(subUUID)) {
                      umundo::SubscriberStub subStub(boost::shared_ptr<umundo::SubscriberStubImpl>(new umundo::SubscriberStubImpl()));
                      subStub.getImpl()->setChannelName(subChannel);
                      subStub.getImpl()->setUUID(subUUID);
                      nodeStub.addSubscriber(subStub);
                    }
                    processNodeCommSubscriptions(nodeStub, nodeStub.getSubscriber(subUUID));
                  } else {
                    if (!nodeStub.getSubscriber(subUUID)) {
                      LOG_WARN("Got an unsubscription from an unknown subscriber!");
                      break;
                    }
                    processNodeCommUnsubscriptions(nodeStub, nodeStub.getSubscriber(subUUID));
                  }
                  break;
                }
                default:
                  LOG_WARN("Received unknown message type");
              }
            }
            
            zmq_msg_close (&message);
            if (!more)
              free(remoteUUID);
              remoteUUID = NULL;
              break;      //  Last message part
          }
        }
      }
    }
  
  void processNodeCommPubAdded(const std::string& nodeUUID, const umundo::PublisherStub& pubStub) {
    if (_nodes.find(nodeUUID) != _nodes.end()) {
      // we already know the node
      confirmPubAdded(_nodes[nodeUUID], pubStub);
    } else {
      _pendingRemotePubs[nodeUUID][pubStub.getUUID()] = pubStub;
    }
  }

  void confirmPubAdded(umundo::NodeStub& nodeStub, const umundo::PublisherStub& pubStub) {
    pubStub.getImpl()->setHost(nodeStub.getHost());
    pubStub.getImpl()->setTransport(nodeStub.getTransport());
    pubStub.getImpl()->setIP(nodeStub.getIP());
    pubStub.getImpl()->setRemote(nodeStub.isRemote());
    pubStub.getImpl()->setInProcess(nodeStub.isInProcess());
    nodeStub.addPublisher(pubStub);
    
    // see if we have a local subscriber interested in the publisher's channel
    std::map<std::string, umundo::Subscriber>::iterator subIter;
    for (subIter = _subs.begin(); subIter != _subs.end(); subIter++) {
      if(pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
        subIter->second.added(pubStub);
        sendSubAdded(_sockets[nodeStub.getUUID()], subIter->second, pubStub, false);
      }
    }

  }
  
  void processPubRemoval(const std::string& nodeUUID, const umundo::PublisherStub& pubStub) {
    std::map<std::string, umundo::Subscriber>::iterator subIter = _subs.begin();
    while(subIter != _subs.end()) {
      if(pubStub.getChannelName().substr(0, subIter->second.getChannelName().size()) == subIter->second.getChannelName()) {
        subIter->second.removed(pubStub);
      }
      subIter++;
    }
  }

  void processZMQCommSubscriptions(const std::string channel) {
    // we have a subscription from zeromq, try to confirm pending subscriptions from node communication
    std::multimap<std::string, std::pair<long, std::pair<umundo::NodeStub, umundo::SubscriberStub> > >::iterator subIter = _pendingSubscriptions.find(channel);
    while(subIter != _pendingSubscriptions.end()) {
      std::pair<umundo::NodeStub, umundo::SubscriberStub> subPair = subIter->second.second;
      if (confirmSubscription(subPair.first, subPair.second))
        return;
      subIter++;
    }
  }
  
  void processNodeCommSubscriptions(const umundo::NodeStub& nodeStub, const umundo::SubscriberStub& subStub) {
    subStub.getImpl()->setLastSeen(umundo::Thread::getTimeStampMs());
    std::pair<umundo::NodeStub, umundo::SubscriberStub> subPair = std::make_pair(nodeStub, subStub);
    long now = umundo::Thread::getTimeStampMs();
    std::pair<long, std::pair<umundo::NodeStub, umundo::SubscriberStub> > timedSubPair = std::make_pair(now, subPair);
    
    std::string subId = std::string("um.sub." + subStub.getUUID());
    _pendingSubscriptions.insert(std::make_pair(subId, timedSubPair));
    
    confirmSubscription(nodeStub, subStub);
  }
  
  void processNodeCommUnsubscriptions(umundo::NodeStub& nodeStub, const umundo::SubscriberStub& subStub) {
    subStub.getImpl()->setLastSeen(umundo::Thread::getTimeStampMs());
    std::string subId = std::string("um.sub." + subStub.getUUID());
    std::string channel = subStub.getChannelName();

    if (_confirmedSubscribers.count(subId) > 0) {
      _confirmedSubscribers.erase(subId);
    } else {
      LOG_WARN("Trying to remove unconfirmed subscriber: %s", subId.c_str());
    }
    if (_confirmedSubscribers.count(channel) > 0) {
      _confirmedSubscribers.erase(channel);
    } else {
      LOG_WARN("Trying to remove unconfirmed subscription: %s", channel.c_str());
    }

    std::map<std::string, umundo::Publisher>::iterator pubIter = _pubs.begin();
    while(pubIter != _pubs.end()) {
      if(pubIter->second.getChannelName().substr(0, subStub.getChannelName().size()) == subStub.getChannelName()) {
        pubIter->second.removedSubscriber(nodeStub.getUUID(), subStub.getUUID());
      }
      pubIter++;
    }
  }
  
  bool confirmSubscription(const umundo::NodeStub& nodeStub, const umundo::SubscriberStub& subStub) {
    // see whether we have all the subscribers subscriptions
    std::string subId = std::string("um.sub." + subStub.getUUID());
    
    int receivedSubs;
    int confirmedSubs;
    receivedSubs = _subscriptions.count(subId);
    confirmedSubs = _confirmedSubscribers.count(subId);
    if (receivedSubs <= confirmedSubs)
      return false;
    
    // we received all zeromq subscriptions for this node, notify publishers
    _confirmedSubscribers.insert(subId);

    // remove from pending subscriptions
    std::multimap<std::string, std::pair<long, std::pair<umundo::NodeStub, umundo::SubscriberStub> > >::iterator subIter;
    subIter = _pendingSubscriptions.find(subId);
    while(subIter != _pendingSubscriptions.end()) {
      std::pair<umundo::NodeStub, umundo::SubscriberStub> subPair = subIter->second.second;
      if (subPair.first == nodeStub && subPair.second == subStub) _pendingSubscriptions.erase(subIter++);
      else subIter++;
    }
    
    nodeStub.getImpl()->addSubscriber(subStub);

    std::map<std::string, umundo::Publisher>::iterator pubIter = _pubs.begin();
    while(pubIter != _pubs.end()) {
      if(pubIter->second.getChannelName().substr(0, subStub.getChannelName().size()) == subStub.getChannelName()) {
        pubIter->second.addedSubscriber(nodeStub.getUUID(), subStub.getUUID());
      }
      pubIter++;
    }

    return true;
  }

  /**
   * Write channel\0uuid\0port into the given byte array
   */
  char* writePubInfo(char* buffer, const umundo::PublisherStub& pub) {
    const char* channel = pub.getChannelName().c_str();
    const char* uuid = pub.getUUID().c_str(); // we share a publisher socket in the node, do not publish hidden pub uuid
    uint16_t port = _pubPort; //pub.getPort();
    
    char* start = buffer;
    (void)start; // surpress unused warning wiithout assert
    
    memcpy(buffer, channel, strlen(channel) + 1);      buffer += strlen(channel) + 1;
    memcpy(buffer, uuid, strlen(uuid) + 1);            buffer += strlen(uuid) + 1;
    *(uint16_t*)buffer = htons(port);                  buffer += 2;
    
    assert(buffer - start == PUB_INFO_SIZE(pub));
    return buffer;
  }
  
  char* readPubInfo(char* buffer, uint16_t& port, char*& channelName, char*& uuid) {
    char* start = buffer;
    (void)start; // surpress unused warning without assert
    
    channelName = buffer;               buffer += strlen(buffer) + 1;    
    uuid = buffer;                      buffer += strlen(buffer) + 1;
    port = ntohs(*(short*)(buffer));    buffer += 2;
    
    assert(buffer - start == (int)strlen(channelName) + 1 + (int)strlen(uuid) + 1 + 2);
    return buffer;
  }

  char* writeSubInfo(char* buffer, const umundo::Subscriber& sub) {
    const char* channel = sub.getChannelName().c_str();
    const char* uuid = sub.getUUID().c_str();
    
    char* start = buffer;
    (void)start; // surpress unused warning without assert
    
    memcpy(buffer, channel, strlen(channel) + 1);     buffer += strlen(channel) + 1;
    memcpy(buffer, uuid, strlen(uuid) + 1);           buffer += strlen(uuid) + 1;

    assert(buffer - start == SUB_INFO_SIZE(sub));
    return buffer;
  }
  
  char* readSubInfo(char* buffer, char*& channelName, char*& uuid) {
    char* start = buffer;
    (void)start; // surpress unused warning without assert

    channelName = buffer;               buffer += strlen(channelName) + 1;
    uuid = buffer;                      buffer += strlen(uuid) + 1;

    assert(buffer - start == (int)strlen(channelName) + 1 + (int)strlen(uuid) + 1);
    return buffer;
  }
};

#if 0
bool testThroughput() {
  receptions = 0;
  ZeroMQNode* zmqNode1 = new ZeroMQNode();
  ZeroMQNode* zmqNode2 = new ZeroMQNode();
  
  umundo::Publisher pub1(boost::shared_ptr<umundo::PublisherImpl>(new ZeroMQPublisher(context, "test.")));
  umundo::Publisher pub2(boost::shared_ptr<umundo::PublisherImpl>(new ZeroMQPublisher(context, "test.some.more")));

  umundo::Subscriber sub1(boost::shared_ptr<umundo::SubscriberImpl>(new ZeroMQSubscriber(context, "tes")));

  
  zmqNode1->addPublisher(pub1);
  zmqNode1->addPublisher(pub2);
  zmqNode2->addSubscriber(sub1);
  
  pub1.waitForSubscribers(1);
  pub2.waitForSubscribers(1);

  long now = umundo::Thread::getTimeStampMs();
  int iterations = 10;
  const char* message = "Hello";
  for(int i = 0; i < iterations; i++) {
    pub1.send(message, strlen(message) + 1);
    pub2.send(message, strlen(message) + 1);
  }
  
  int retries = 1000;
  while(receptions < iterations * 2 && retries--) {
    umundo::Thread::sleepMs(20);
  }
  
  long finished = umundo::Thread::getTimeStampMs();
  printf("%i packets received in %li ms\n", receptions, (finished - now));

  zmqNode2->removeSubscriber(sub1);

  umundo::Thread::sleepMs(500);

  delete zmqNode1;
  delete zmqNode2;
  
  return true;
}
#endif
#if 0
bool testScalability() {
  receptions = 0;
  int nrSubscribers = 10;
  int nrMessages = 10;
  std::set<ZeroMQSubscriber*> subs;
  umundo::Publisher pub1(boost::shared_ptr<umundo::PublisherImpl>(new ZeroMQPublisher(context, "test.")));
  ZeroMQNode* zmqNode = new ZeroMQNode("domain");
  zmqNode->addPublisher(pub1);
  
  for (int i = 0; i < nrSubscribers; i++) {
    ZeroMQSubscriber* sub = new ZeroMQSubscriber(context, "test.");
    subs.insert(sub);
  }

  umundo::Thread::sleepMs(500);

  long now = umundo::Thread::getTimeStampMs();
  const char* message = "Hello";
  for (int i = 0; i < nrMessages; i++) {
    zmqPub1->send(message, strlen(message) + 1);
  }
  
  int retries = 300;
  while(receptions != nrSubscribers * nrMessages && retries--) {
    umundo::Thread::sleepMs(20);
  }
  long finished = umundo::Thread::getTimeStampMs();
  printf("%i packets received in %li ms\n", receptions, (finished - now));
  
  std::set<ZeroMQSubscriber*>::iterator subIter = subs.begin();
  while(subIter != subs.end()) {
    delete *subIter;
    subIter++;
  }
  
  delete zmqNode;
  delete zmqPub1;
  return true;
}
#endif

int main(int argc, char** argv) {
  context = zmq_ctx_new();
  
//  if (!testThroughput())
//    return EXIT_FAILURE;
//  if (!testScalability())
//    return EXIT_FAILURE;
  
  zmq_ctx_destroy(context);
  return EXIT_SUCCESS;
}