%module(directors="1", allprotected="1") umundoNativeCSharp

// import swig typemaps
//%include <arrays_csharp.i>
%include <stl.i>
%include <inttypes.i>
%include "../stl_set.i"

// macros from cmake
%import "umundo/config.h"

// set UMUNDO_API macro to empty string
#define UMUNDO_API

%rename(equals) operator==; 
%rename(isValid) operator bool;
%rename(waitSignal) wait;

%csconst(1);

//**************************************************
// This ends up in the generated wrapper code
//**************************************************

%{
#include "../../../../core/src/umundo/common/Host.h"
#include "../../../../core/src/umundo/common/EndPoint.h"
#include "../../../../core/src/umundo/connection/NodeStub.h"
#include "../../../../core/src/umundo/connection/Node.h"
#include "../../../../core/src/umundo/common/Message.h"
#include "../../../../core/src/umundo/common/Regex.h"
#include "../../../../core/src/umundo/common/ResultSet.h"
#include "../../../../core/src/umundo/thread/Thread.h"
#include "../../../../core/src/umundo/connection/PublisherStub.h"
#include "../../../../core/src/umundo/connection/SubscriberStub.h"
#include "../../../../core/src/umundo/connection/Publisher.h"
#include "../../../../core/src/umundo/connection/Subscriber.h"
#include "../../../../core/src/umundo/discovery/Discovery.h"


using namespace umundo;
%}


//*************************************************/

// allow CSharp classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;

%include "../umundo_ignores.i"
%include "../umundo_beautify.i"


//******************************
// WARNING - Prevent premature GC!
//******************************

// this is helpful:
// http://stackoverflow.com/questions/9817516/swig-java-retaining-class-information-of-the-objects-bouncing-from-c

// The CSharp GC will eat receivers and greeters as the wrapper code never
// holds a reference to their CSharp objects. See respective typemaps for
// Publisher and Subscriber below


//***************
// Always copy messages into the VM

# messages are destroyed upon return, always pass copies to CSharp
# %typemap(csdirectorin) umundo::Message* "(msg == 0) ? null : new Message(new Message(msg, false))"


//******************************
// Beautify Node class
//******************************

%rename(getSubscribersNative) umundo::Node::getSubscribers();
%csmethodmodifiers umundo::Node::getSubscribers() "private";
%rename(getPublishersNative) umundo::Node::getPublishers();
%csmethodmodifiers umundo::Node::getPublishers() "private";

%typemap(csimports) umundo::Node %{
  using System;
  using System.Collections.Generic;
  using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::Node %{
  public Dictionary<string, Publisher> getPublishers() {
  	Dictionary<string, Publisher> pubs = new Dictionary<string, Publisher>();
  	PublisherMap pubMap = getPublishersNative();
  	StringArray pubKeys = getPubKeys();
  	for (int i = 0; i < pubKeys.Count; i++) {
  		pubs[pubKeys[i]] = pubMap[pubKeys[i]];
  	}
  	return pubs;
  }
  public Dictionary<string, Subscriber> getSubscribers() {
  	Dictionary<string, Subscriber> subs = new Dictionary<string, Subscriber>();
  	SubscriberMap subMap = getSubscribersNative();
  	StringArray subKeys = getSubKeys();
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

%typemap(csimports) umundo::NodeStub %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::NodeStub %{
	public Dictionary<string, PublisherStub> getPublishers() {
		Dictionary<string, PublisherStub> pubs = new Dictionary<string, PublisherStub>();
		PublisherStubMap pubMap = getPublishersNative();
		StringArray pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.Count; i++) {
			pubs[pubKeys[i]] = pubMap[pubKeys[i]];
		}
		return pubs;
	}
	public Dictionary<string, SubscriberStub> getSubscribers() {
		Dictionary<string, SubscriberStub> subs = new Dictionary<string, SubscriberStub>();
		SubscriberStubMap subMap = getSubscribersNative();
		StringArray subKeys = getSubKeys();
		for (int i = 0; i < subKeys.Count; i++) {
			subs[subKeys[i]] = subMap[subKeys[i]];
		}
		return subs;
	}

%}

//******************************
// Beautify Publisher class
//******************************

%rename(setGreeterNative) umundo::Publisher::setGreeter(Greeter*);
%csmethodmodifiers umundo::Publisher::setGreeter(Greeter* greeter) "private";
%rename(getSubscribersNative) umundo::Publisher::getSubscribers();
%csmethodmodifiers umundo::Publisher::getSubscribers() "private";

%typemap(csimports) umundo::Publisher %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::Publisher %{
  // keep receiver as a reference to prevent premature GC
  private Greeter _greeter;

  public void setGreeter(Greeter greeter) {
    // it is important to keep the reference, otherwise the CSharp GC will eat it!
    _greeter = greeter;
    setGreeterNative(greeter);
  }

  public Dictionary<string, SubscriberStub> getSubscribers() {
  	Dictionary<string, SubscriberStub> subs = new Dictionary<string, SubscriberStub>();
  	SubscriberStubMap subMap = getSubscribersNative();
  	StringArray subKeys = getSubKeys();
  	for (int i = 0; i < subKeys.Count; i++) {
  		subs[subKeys[i]] = subMap[subKeys[i]];
  	}
  	return subs;
  }
%}


//******************************
// Beautify Subscriber class
//******************************

%rename(getPublishersNative) umundo::Subscriber::getPublishers();
%csmethodmodifiers umundo::Subscriber::getPublishers() "private";

// Do not generate this constructor - substitute by the one in the cscode typemap below
%ignore umundo::Subscriber::Subscriber(const std::string&, Receiver*);

// hide this constructor to enforce the one below
%csmethodmodifiers umundo::Subscriber::Subscriber(string channelName) "protected";

// rename setter and wrap by setter in cscode typemap below
%rename(setReceiverNative) umundo::Subscriber::setReceiver(Receiver*);
%csmethodmodifiers umundo::Subscriber::setReceiver(Receiver* receiver) "private";

%typemap(csimports) umundo::Subscriber %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::Subscriber %{
  // keep receiver as a reference to prevent premature GC
  private Receiver _receiver;

  public Subscriber(string channelName, Receiver receiver) : this(umundoNativeCSharpPINVOKE.new_Subscriber__SWIG_2(channelName), true) {
    setReceiver(receiver);
  }
  public void setReceiver(Receiver receiver) {
    // it is important to keep the reference, otherwise the C# GC will eat it!
    _receiver = receiver;
    setReceiverNative(receiver);
  }

	public Dictionary<string, PublisherStub> getPublishers() {
		Dictionary<string, PublisherStub> pubs = new Dictionary<string, PublisherStub>();
		PublisherStubMap pubMap = getPublishersNative();
		StringArray pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.Count; i++) {
			pubs[pubKeys[i]] = pubMap[pubKeys[i]];
		}
		return pubs;
	}

%}

//******************************
// Beautify Regex class
//******************************

%rename(getSubMatchesNative) umundo::Regex::getSubMatches();
%csmethodmodifiers umundo::Regex::getSubMatches() "private";

%typemap(csimports) umundo::Regex %{
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
%}

%typemap(cscode) umundo::Regex %{

	public List<string> getSubMatches() {
		List<string> subMatches = new List<string>();
		StringArray subMatchesNative = getSubMatchesNative();
		for (int i = 0; i < subMatchesNative.Count; i++) {
			subMatches.Add(subMatchesNative[i]);
		}
		return subMatches;
	}
%}


//******************************
// Beautify Message class
//******************************

%ignore umundo::Message::Message(string);
//%ignore umundo::Message::Message(const Message&);
%ignore umundo::Message::setData(const std::string&);
%ignore umundo::Message::isQueued();
%ignore umundo::Message::setQueued(bool);
%ignore umundo::Message::typeToString(uint16_t type);
%csmethodmodifiers umundo::Message::getKeys() "private";

%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;
%rename(getMetaNative) umundo::Message::getMeta();
%csmethodmodifiers umundo::Message::getMeta() "private";
%csmethodmodifiers setData(const char *data, size_t length) "private"

// byte[] signature for get/setData
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

// provide convinience methods within Message CSharp class for meta keys
%typemap(cscode) umundo::Message %{
  public void setData(byte[] buffer) {
  	setData(buffer, (uint)buffer.Length);
  }

  public System.Collections.Generic.Dictionary<string, string> getMeta() {
  	System.Collections.Generic.Dictionary<string, string> keys = new System.Collections.Generic.Dictionary<string, string>();
    StringArray metaKeys = getMetaKeys();
  	foreach (string key in metaKeys) {
  		keys[key] = getMeta(key);
  	}
  	return keys;
  }
%}

//******************************
// Make some C++ classes package local
//******************************

//%pragma(cs) moduleclassmodifiers="public final class"
%pragma(cs) jniclassclassmodifiers="class"
%pragma(cs) moduleclassmodifiers="class"
%typemap(csclassmodifiers) SWIGTYPE "public class"

// not working :(
// %template(PublisherSet) std::set<umundo::Publisher>;
// %typemap(csclassmodifiers) PublisherSet "class"
// %typemap(csclassmodifiers) std::set<umundo::Publisher> "class"


//***********************************************
// Parse the header file to generate wrappers
//***********************************************

%include "../../../../core/src/umundo/common/Host.h"
%include "../../../../core/src/umundo/common/Message.h"
%include "../../../../core/src/umundo/thread/Thread.h"
%include "../../../../core/src/umundo/common/Implementation.h"
%include "../../../../core/src/umundo/common/EndPoint.h"
%include "../../../../core/src/umundo/common/Regex.h"
%include "../../../../core/src/umundo/common/ResultSet.h"
%include "../../../../core/src/umundo/connection/PublisherStub.h"
%include "../../../../core/src/umundo/connection/SubscriberStub.h"
%include "../../../../core/src/umundo/connection/NodeStub.h"
%include "../../../../core/src/umundo/connection/Publisher.h"
%include "../../../../core/src/umundo/connection/Subscriber.h"
%include "../../../../core/src/umundo/connection/Node.h"
%include "../../../../core/src/umundo/discovery/Discovery.h"

