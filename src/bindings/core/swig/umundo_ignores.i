//******************************
// ignore these functions in every class
//******************************
%ignore setChannelName(string);
%ignore setUUID(string);
/*%ignore setPort(uint16_t);*/
/*%ignore setIP(string);*/
%ignore setTransport(string);
%ignore setRemote(bool);
%ignore setHost(string);
%ignore setDomain(string);
%ignore getImpl();
%ignore getImpl() const;

//******************************
// ignore class specific functions
//******************************
%ignore operator!=(NodeStub* n) const;
%ignore operator<<(std::ostream&, const NodeStub*);
%ignore operator<<(std::ostream&, const EndPoint&);

//******************************
// ignore operators
//******************************
%ignore operator!=;
%ignore operator<;
%ignore operator=;

//******************************
// Ignore whole C++ classes
//******************************
%ignore Implementation;
%ignore Configuration;
%ignore EndPointImpl;
%ignore DiscoveryImpl;
%ignore NodeImpl;
%ignore NodeStubImpl;
%ignore NodeStubBaseImpl;
%ignore PublisherImpl;
%ignore PublisherStubImpl;
%ignore SubscriberImpl;
%ignore SubscriberStubImpl;
%ignore EndPointImpl;
%ignore RMutex;
%ignore Mutex;
%ignore Thread;
%ignore Monitor;
%ignore MemoryBuffer;
%ignore RScopeLock;

//******************************
// Ignore PIMPL Constructors
//******************************

%ignore Node(const SharedPtr<NodeImpl>);
%ignore Node(const Node&);
%ignore NodeStub(const SharedPtr<NodeStubImpl>);
%ignore NodeStub(const NodeStub&);
%ignore NodeStubBase(const SharedPtr<NodeStubBaseImpl>);
%ignore NodeStubBase(const NodeStubBase&);

%ignore EndPoint(const SharedPtr<EndPointImpl>);
%ignore EndPoint(const EndPoint&);
%ignore EndPoint();

%ignore Publisher(const SharedPtr<PublisherImpl>);
%ignore Publisher(const Publisher&);
%ignore PublisherStub(const SharedPtr<PublisherStubImpl>);
%ignore PublisherStub(const PublisherStub&);

%ignore Subscriber(const SharedPtr<SubscriberImpl>);
%ignore Subscriber(const Subscriber&);
%ignore SubscriberStub(const SharedPtr<SubscriberStubImpl>);
%ignore SubscriberStub(const SubscriberStub&);

%ignore Discovery(const SharedPtr<DiscoveryImpl>);
%ignore Discovery(const Discovery&);
%ignore Discovery();

%ignore umundo::Options::getKVPs;
