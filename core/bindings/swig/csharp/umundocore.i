#ifdef 64BIT_HOST
#	ifdef DEBUG
%module(directors="1", allprotected="1") umundoNativeCSharp64_d
# else
%module(directors="1", allprotected="1") umundoNativeCSharp64
#	endif
#else
#	ifdef DEBUG
%module(directors="1", allprotected="1") umundoNativeCSharp_d
#	else
%module(directors="1", allprotected="1") umundoNativeCSharp
#	endif
#endif

// import swig typemaps
%include <arrays_csharp.i>
%include <stl.i>
%include <std_map.i>
%include <inttypes.i>
%include "stl_set.i"

// macros from cmake
%import "umundo/config.h"

// set DLLEXPORT macro to empty string
#define DLLEXPORT

// SWIG does not recognize 'using std::string' from an include
typedef std::string string;
typedef std::vector vector;
typedef std::set set;

%rename(Equals) operator==; 
%rename(IsValid) operator bool;
%ignore operator!=;
%ignore operator<;
%ignore operator=;

%csconst(1);

//**************************************************
// This ends up in the generated wrapper code
//**************************************************

%{
#include "../../../../core/src/umundo/common/EndPoint.h"
#include "../../../../core/src/umundo/connection/Node.h"
#include "../../../../core/src/umundo/common/Message.h"
#include "../../../../core/src/umundo/thread/Thread.h"
#include "../../../../core/src/umundo/connection/Publisher.h"
#include "../../../../core/src/umundo/connection/Subscriber.h"

using std::string;
using std::vector;
using std::map;
using boost::shared_ptr;
using namespace umundo;
%}

//*************************************************/

// Provide a nicer CSharp interface to STL containers
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

// allow CSharp classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;

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


//******************************
// Prevent premature GC
//******************************

// this is helpful:
// http://stackoverflow.com/questions/9817516/swig-java-retaining-class-information-of-the-objects-bouncing-from-c

// The Java GC will eat receivers and greeters as the wrapper code never
// holds a reference to their Java objects.

//***************
// Save the receiver in the subscriber

// Do not generate this constructor - substitute by the one in the javacode typemap below
%ignore umundo::Subscriber::Subscriber(const std::string&, Receiver*);

// hide this constructor to enforce the one below
%csmethodmodifiers umundo::Subscriber::Subscriber(string channelName) "protected";

// rename setter an wrap by setter in javacode typemap below
%rename(setReceiverNative) umundo::Subscriber::setReceiver(Receiver*);
%csmethodmodifiers umundo::Subscriber::setReceiver(Receiver* receiver) "private";

%typemap(cscode) umundo::Subscriber %{
  // keep receiver as a reference to prevent premature GC
  private Receiver _receiver;

  public Subscriber(string channelName, Receiver receiver) : this(umundoNativeCSharp64PINVOKE.new_Subscriber__SWIG_2(channelName), true) {
    setReceiver(receiver);
  }

  public void setReceiver(Receiver receiver) {
    // it is important to keep the reference, otherwise the Java GC will eat it!
    _receiver = receiver;
    setReceiverNative(receiver);
  }
%}

//***************
// Save the greeter in the publisher (same approach as above)

%rename(setGreeterNative) umundo::Publisher::setGreeter(Greeter*);
%csmethodmodifiers umundo::Publisher::setGreeter(Greeter* greeter) "private";

%typemap(cscode) umundo::Publisher %{
  // keep receiver as a reference to prevent premature GC
  private Greeter _greeter;

  public void setGreeter(Greeter greeter) {
    // it is important to keep the reference, otherwise the CSharp GC will eat it!
    _greeter = greeter;
    setGreeterNative(greeter);
  }
%}

//******************************
// Beautify Message class
//******************************

// ignore ugly std::map return
%ignore umundo::Message::getMeta();
//%ignore umundo::Message::Message(const Message&);
%ignore umundo::Message::setData(string const &);
%ignore umundo::Message::Message(string);
%ignore umundo::Message::Message(const char*, size_t);
%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;

//******************************
// Beautify Node class
//******************************

%rename(getSubscribersNative) umundo::Node::getSubscribers();
%csmethodmodifiers umundo::Node::getSubscribers() "private";
%rename(getPublishersNative) umundo::Node::getPublishers();
%csmethodmodifiers umundo::Node::getPublishers() "private";

%extend umundo::Node {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Publisher>::iterator pubIter = self->getPublishers().begin();
		while(pubIter != self->getPublishers().end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Subscriber>::iterator subIter = self->getSubscribers().begin();
		while(subIter != self->getSubscribers().end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};

%typemap(csimports) umundo::Node %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::Node %{
	public Dictionary<string, Publisher> getPublishers() {
		Dictionary<string, Publisher> pubs = new Dictionary<string, Publisher>();
		PublisherMap pubMap = getPublishersNative();
		StringVector pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.Count; i++) {
			pubs[pubKeys[i]] = pubMap[pubKeys[i]];
		}
		return pubs;
	}
	public Dictionary<string, Subscriber> getSubscribers() {
		Dictionary<string, Subscriber> subs = new Dictionary<string, Subscriber>();
		SubscriberMap subMap = getSubscribersNative();
		StringVector subKeys = getSubKeys();
		for (int i = 0; i < subKeys.Count; i++) {
			subs[subKeys[i]] = subMap[subKeys[i]];
		}
		return subs;
	}

%}

//******************************
// Beautify NodeStub class
//******************************

%rename(getSubscribersNative) umundo::NodeStub::getSubscribers();
%csmethodmodifiers umundo::NodeStub::getSubscribers() "private";
%rename(getPublishersNative) umundo::NodeStub::getPublishers();
%csmethodmodifiers umundo::NodeStub::getPublishers() "private";

%extend umundo::NodeStub {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, PublisherStub>::iterator pubIter = self->getPublishers().begin();
		while(pubIter != self->getPublishers().end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, SubscriberStub>::iterator subIter = self->getSubscribers().begin();
		while(subIter != self->getSubscribers().end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};


%typemap(csimports) umundo::NodeStub %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::NodeStub %{
	public Dictionary<string, PublisherStub> getPublishers() {
		Dictionary<string, PublisherStub> pubs = new Dictionary<string, PublisherStub>();
		PublisherStubMap pubMap = getPublishersNative();
		StringVector pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.Count; i++) {
			pubs[pubKeys[i]] = pubMap[pubKeys[i]];
		}
		return pubs;
	}
	public Dictionary<string, SubscriberStub> getSubscribers() {
		Dictionary<string, SubscriberStub> subs = new Dictionary<string, SubscriberStub>();
		SubscriberStubMap subMap = getSubscribersNative();
		StringVector subKeys = getSubKeys();
		for (int i = 0; i < subKeys.Count; i++) {
			subs[subKeys[i]] = subMap[subKeys[i]];
		}
		return subs;
	}

%}

//******************************
// byte[] signature for get/setData
//******************************

// see http://permalink.gmane.org/gmane.comp.programming.swig/5804
%typemap(imtype, out="IntPtr") const char *data "byte[]"
%typemap(cstype) const char *data "byte[]"
%typemap(in) const char *data %{ $1 = ($1_ltype)$input; %}
%typemap(csin) const char *data "$csinput"

%typemap(csout) const char *data %{
	{
    byte[] ret = new byte[this.getSize()]; 
    IntPtr data = $imcall;
    System.Runtime.InteropServices.Marshal.Copy(data, ret, 0, (int)this.getSize());
    return ret; 
  } 
%} 

// make sure we do not get the default with SWIG_csharp_string_callback
%typemap(out) const char *data {
  $result = (char *)result;
}


// import java.util.HashMap
// %typemap(csimports) umundo::Message %{
// using System.Collections.Generic.Dictionary;
// %}

// provide convinience methods within Message C# class for meta keys
%typemap(cscode) umundo::Message %{

	public void setData(byte[] buffer) {
  	setData(buffer, (uint)buffer.Length);
	}

	public System.Collections.Generic.Dictionary<string, string> getMeta() {
		System.Collections.Generic.Dictionary<string, string> keys = new System.Collections.Generic.Dictionary<string, string>();
		foreach (string key in getKeys()) {
			keys[key] = getMeta(key);
		}
		return keys;
	}
%}

%csmethodmodifiers setData(const char *data, size_t length) "private"

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
%ignore Mutex;
%ignore Thread;
%ignore Monitor;
%ignore MemoryBuffer;
%ignore ScopeLock;

//******************************
// Ignore PIMPL Constructors
//******************************

%ignore Node(const boost::shared_ptr<NodeImpl>);
%ignore Node(const Node&);
%ignore NodeStub(const boost::shared_ptr<NodeStubImpl>);
%ignore NodeStub(const NodeStub&);
%ignore NodeStubBase(const boost::shared_ptr<NodeStubBaseImpl>);
%ignore NodeStubBase(const NodeStubBase&);

%ignore EndPoint(const boost::shared_ptr<EndPointImpl>);
%ignore EndPoint(const EndPoint&);

%ignore Publisher(const boost::shared_ptr<PublisherImpl>);
%ignore Publisher(const Publisher&);
%ignore PublisherStub(const boost::shared_ptr<PublisherStubImpl>);
%ignore PublisherStub(const PublisherStub&);

%ignore Subscriber(const boost::shared_ptr<SubscriberImpl>);
%ignore Subscriber(const Subscriber&);
%ignore SubscriberStub(const boost::shared_ptr<SubscriberStubImpl>);
%ignore SubscriberStub(const SubscriberStub&);

//***********************************************
// Parse the header file to generate wrappers
//***********************************************

%include "../../../../core/src/umundo/common/Message.h"
%include "../../../../core/src/umundo/thread/Thread.h"
%include "../../../../core/src/umundo/common/Implementation.h"
%include "../../../../core/src/umundo/common/EndPoint.h"
%include "../../../../core/src/umundo/connection/Publisher.h"
%include "../../../../core/src/umundo/connection/Subscriber.h"
%include "../../../../core/src/umundo/connection/Node.h"

