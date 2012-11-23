%module(directors="1", allprotected="1") umundoNativeCSharp
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
%template(PublisherStubSet) std::set<umundo::PublisherStub>;
%template(SubscriberStubSet) std::set<umundo::SubscriberStub>;

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

