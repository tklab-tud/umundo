#ifdef HOST_64BIT
#	ifdef DEBUG
%module(directors="1", allprotected="1") umundoNativePython64_d
# else
%module(directors="1", allprotected="1") umundoNativePython64
#	endif
#else
#	ifdef DEBUG
%module(directors="1", allprotected="1") umundoNativePython_d
#	else
%module(directors="1", allprotected="1") umundoNativePython
#	endif
#endif

%include <std_string.i>
%include <std_pair.i>
%include <std_map.i>
%include <std_set.i>
%include <stl.i>

// macros from cmake
%import "umundo/config.h"

// set UMUNDO_API macro to empty string
#define UMUNDO_API

// SWIG does not recognize 'using std::string' from an include
typedef std::string string;
typedef std::vector vector;
typedef std::map map;
typedef std::set set;
typedef std::list list;

%rename(equals) operator==; 
%rename(isValid) operator bool;
%ignore operator!=;
%ignore operator<;
%ignore operator=;

//**************************************************
// This ends up in the generated wrapper code
//**************************************************

%{
#include "../../../../core/src/umundo/common/EndPoint.h"
#include "../../../../core/src/umundo/connection/Node.h"
#include "../../../../core/src/umundo/common/Message.h"
#include "../../../../core/src/umundo/common/Regex.h"
#include "../../../../core/src/umundo/thread/Thread.h"
#include "../../../../core/src/umundo/connection/Publisher.h"
#include "../../../../core/src/umundo/connection/Subscriber.h"

using std::string;
using std::vector;
using std::map;
using SharedPtr;
using namespace umundo;
%}

//*************************************************/

// Provide a nicer Python interface to STL containers
%template(StringVector) std::vector<std::string>;
%template(StringSet)    std::set<std::string>;
%template(PublisherSet) std::set<umundo::Publisher>;
%template(SubscriberSet) std::set<umundo::Subscriber>;
%template(PublisherMap) std::map<std::string, umundo::Publisher>;
%template(SubscriberMap) std::map<std::string, umundo::Subscriber>;
%template(PublisherStubSet) std::set<umundo::PublisherStub>;
%template(SubscriberStubSet) std::set<umundo::SubscriberStub>;
%template(PublisherStubMap) std::map<std::string, umundo::PublisherStub>;
%template(SubscriberStubMap) std::map<std::string, umundo::SubscriberStub>;

// allow Python classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;

// enable conversion from char*, int to jbytearray
%apply (char *STRING, size_t LENGTH) { (const char* data, size_t length) };


# %extend umundo::Message {
# 	void getData(char **result, int *len) {
# 		*result = (char *)self->data();
# 		int size = self->size();
# 		len = &size;
# 	}
# }

// ignore these functions in every class
%ignore setChannelName(string);
%ignore setUUID(string);
%ignore setPort(uint16_t);
%ignore setIP(string);
%ignore setTransport(string);
%ignore setRemote(bool);
%ignore setHost(string);
%ignore setDomain(string);
%ignore getImpl();
%ignore getImpl() const;

// ignore class specific functions
%ignore operator!=(NodeStub* n) const;
%ignore operator<<(std::ostream&, const NodeStub*);

// rename functions
%rename(equals) operator==(NodeStub* n) const;
%rename(waitSignal) wait;
%rename(yieldThis) yield;

//******************************
// Ignore whole C++ classes
//******************************

%ignore Implementation;
%ignore Configuration;
%ignore NodeConfig;
%ignore PublisherConfig;
%ignore SubscriberConfig;
%ignore EndPointImpl;
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

%ignore Publisher(const SharedPtr<PublisherImpl>);
%ignore Publisher(const Publisher&);
%ignore PublisherStub(const SharedPtr<PublisherStubImpl>);
%ignore PublisherStub(const PublisherStub&);

%ignore Subscriber(const SharedPtr<SubscriberImpl>);
%ignore Subscriber(const Subscriber&);
%ignore SubscriberStub(const SharedPtr<SubscriberStubImpl>);
%ignore SubscriberStub(const SubscriberStub&);

//***********************************************
// Parse the header file to generate wrappers
//***********************************************

%include "../../../../core/src/umundo/common/Message.h"
%include "../../../../core/src/umundo/thread/Thread.h"
%include "../../../../core/src/umundo/common/Implementation.h"
%include "../../../../core/src/umundo/common/EndPoint.h"
%include "../../../../core/src/umundo/common/Regex.h"
%include "../../../../core/src/umundo/connection/Publisher.h"
%include "../../../../core/src/umundo/connection/Subscriber.h"
%include "../../../../core/src/umundo/connection/Node.h"

