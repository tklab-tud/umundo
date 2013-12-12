%module(directors="1", allprotected="1") umundoNativeJava
// import swig typemaps
// %include "stl_map.i"
%include <arrays_java.i>
%include <stl.i>
%include <inttypes.i>
%include "stl_set.i"

// macros from cmake
%import "umundo/config.h"

// set DLLEXPORT macro to empty string
#define DLLEXPORT

// this needs to be up here for the template
%include "../../../../core/src/umundo/common/ResultSet.h"

%rename(equals) operator==; 
%ignore operator bool;
%ignore operator!=;
%ignore operator<;
%ignore operator=;

%javaconst(1);

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

#ifdef ANDROID
// google forgot imaxdiv in the android ndk r7 libc?!
#ifndef imaxdiv
imaxdiv_t imaxdiv(intmax_t numer, intmax_t denom) {
	imaxdiv_t res;
	res.quot=0; res.rem=0;
	while(numer >= denom) {
		res.quot++;
		numer -= denom;
	}
	res.rem = numer;
	return res;
}
#endif
#endif

using namespace umundo;
%}

#ifndef ANDROID
// auto loading does not work on android as the jar may not contain JNI code
%include "autoloadJNI.i"
#endif

//*************************************************/

// Provide a nicer Java interface to STL containers
%template(StringArray) std::vector<std::string>;
%template(StringMap) std::map<std::string, std::string>;
%template(PublisherMap) std::map<std::string, umundo::Publisher>;
%template(SubscriberMap) std::map<std::string, umundo::Subscriber>;
%template(PublisherStubMap) std::map<std::string, umundo::PublisherStub>;
%template(SubscriberStubMap) std::map<std::string, umundo::SubscriberStub>;
%template(NodeStubMap) std::map<std::string, umundo::NodeStub>;
%template(EndPointResultSet) umundo::ResultSet<umundo::EndPoint>;
%template(EndPointArray) std::vector<umundo::EndPoint>;
%template(InterfaceArray) std::vector<umundo::Interface>;

// allow Java classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;

// enable conversion from char*, int to jbytearray
%apply (char *STRING, size_t LENGTH) { (const char* data, size_t length) };

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
// WARNING - Prevent premature GC!
//******************************

// this is helpful:
// http://stackoverflow.com/questions/9817516/swig-java-retaining-class-information-of-the-objects-bouncing-from-c

// The Java GC will eat receivers and greeters as the wrapper code never
// holds a reference to their Java objects. See respective typemaps for
// Publisher and Subscriber below


//***************
// Always copy messages into the JVM

# messages are destroyed upon return, always pass copies to Java
%typemap(javadirectorin) umundo::Message* "(msg == 0) ? null : new Message(new Message(msg, false))"


//******************************
// Beautify Node class
//******************************

%rename(getSubscribersNative) umundo::Node::getSubscribers();
%javamethodmodifiers umundo::Node::getSubscribers() "private";
%rename(getPublishersNative) umundo::Node::getPublishers();
%javamethodmodifiers umundo::Node::getPublishers() "private";

%extend umundo::Node {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Publisher> pubs = self->getPublishers();
		std::map<std::string, Publisher>::iterator pubIter = pubs.begin();
		while(pubIter != pubs.end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, Subscriber> subs = self->getSubscribers();
		std::map<std::string, Subscriber>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};

%typemap(javaimports) umundo::Node %{
import java.util.HashMap;
%}

%typemap(javacode) umundo::Node %{
	public HashMap<String, Publisher> getPublishers() {
		HashMap<String, Publisher> pubs = new HashMap<String, Publisher>();
		PublisherMap pubMap = getPublishersNative();
		StringArray pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.size(); i++) {
			pubs.put(pubKeys.get(i), pubMap.get(pubKeys.get(i)));
		}
		return pubs;
	}
	public HashMap<String, Subscriber> getSubscribers() {
		HashMap<String, Subscriber> subs = new HashMap<String, Subscriber>();
		SubscriberMap subMap = getSubscribersNative();
		StringArray subKeys = getSubKeys();
		for (int i = 0; i < subKeys.size(); i++) {
			subs.put(subKeys.get(i), subMap.get(subKeys.get(i)));
		}
		return subs;
	}

%}


//******************************
// Beautify NodeStub class
//******************************

%rename(getSubscribersNative) umundo::NodeStub::getSubscribers();
%javamethodmodifiers umundo::NodeStub::getSubscribers() "private";
%rename(getPublishersNative) umundo::NodeStub::getPublishers();
%javamethodmodifiers umundo::NodeStub::getPublishers() "private";

%extend umundo::NodeStub {
  std::vector<std::string> getPubKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, PublisherStub> pubs = self->getPublishers();
  	std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
  	while(pubIter != pubs.end()) {
  		keys.push_back(pubIter->first);
  		pubIter++;
  	}
  	return keys;
  }
  std::vector<std::string> getSubKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, SubscriberStub> subs = self->getSubscribers();
  	std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
  	while(subIter != subs.end()) {
  		keys.push_back(subIter->first);
  		subIter++;
  	}
  	return keys;
  }
};

%typemap(javaimports) umundo::NodeStub %{
import java.util.HashMap;
%}

%typemap(javacode) umundo::NodeStub %{
	public HashMap<String, PublisherStub> getPublishers() {
		HashMap<String, PublisherStub> pubs = new HashMap<String, PublisherStub>();
		PublisherStubMap pubMap = getPublishersNative();
		StringArray pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.size(); i++) {
			pubs.put(pubKeys.get(i), pubMap.get(pubKeys.get(i)));
		}
		return pubs;
	}
	public HashMap<String, SubscriberStub> getSubscribers() {
		HashMap<String, SubscriberStub> subs = new HashMap<String, SubscriberStub>();
		SubscriberStubMap subMap = getSubscribersNative();
		StringArray subKeys = getSubKeys();
		for (int i = 0; i < subKeys.size(); i++) {
			subs.put(subKeys.get(i), subMap.get(subKeys.get(i)));
		}
		return subs;
	}

%}

//******************************
// Beautify Publisher class
//******************************

%rename(setGreeterNative) umundo::Publisher::setGreeter(Greeter*);
%javamethodmodifiers umundo::Publisher::setGreeter(Greeter* greeter) "private";
%rename(getSubscribersNative) umundo::Publisher::getSubscribers();
%javamethodmodifiers umundo::Publisher::getSubscribers() "private";

%extend umundo::Publisher {
	std::vector<std::string> getSubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, SubscriberStub> subs = self->getSubscribers();
		std::map<std::string, SubscriberStub>::iterator subIter = subs.begin();
		while(subIter != subs.end()) {
			keys.push_back(subIter->first);
			subIter++;
		}
		return keys;
	}
};

%typemap(javaimports) umundo::Publisher %{
import java.util.HashMap;
%}

%typemap(javacode) umundo::Publisher %{
  // keep receiver as a reference to prevent premature GC
  private Greeter _greeter;

  public void setGreeter(Greeter greeter) {
    // it is important to keep the reference, otherwise the Java GC will eat it!
    _greeter = greeter;
    setGreeterNative(greeter);
  }

	public HashMap<String, SubscriberStub> getSubscribers() {
		HashMap<String, SubscriberStub> subs = new HashMap<String, SubscriberStub>();
		SubscriberStubMap subMap = getSubscribersNative();
		StringArray subKeys = getSubKeys();
		for (int i = 0; i < subKeys.size(); i++) {
			subs.put(subKeys.get(i), subMap.get(subKeys.get(i)));
		}
		return subs;
	}
%}


//******************************
// Beautify Subscriber class
//******************************

%rename(getPublishersNative) umundo::Subscriber::getPublishers();
%javamethodmodifiers umundo::Subscriber::getPublishers() "private";

// Do not generate this constructor - substitute by the one in the javacode typemap below
%ignore umundo::Subscriber::Subscriber(const std::string&, Receiver*);

// hide this constructor to enforce the one below
%javamethodmodifiers umundo::Subscriber::Subscriber(string channelName) "protected";

// rename setter and wrap by setter in javacode typemap below
%rename(setReceiverNative) umundo::Subscriber::setReceiver(Receiver*);
%javamethodmodifiers umundo::Subscriber::setReceiver(Receiver* receiver) "private";

%extend umundo::Subscriber {
	std::vector<std::string> getPubKeys() {
		std::vector<std::string> keys;
		std::map<std::string, PublisherStub> pubs = self->getPublishers();
		std::map<std::string, PublisherStub>::iterator pubIter = pubs.begin();
		while(pubIter != pubs.end()) {
			keys.push_back(pubIter->first);
			pubIter++;
		}
		return keys;
	}
};

%typemap(javaimports) umundo::Subscriber %{
import java.util.HashMap;
%}

%typemap(javacode) umundo::Subscriber %{
  // keep receiver as a reference to prevent premature GC
  private Receiver _receiver;

  public Subscriber(String channelName, Receiver receiver) {
    this(umundoNativeJavaJNI.new_Subscriber__SWIG_2(channelName), true);
    setReceiver(receiver);
  }

  public void setReceiver(Receiver receiver) {
    // it is important to keep the reference, otherwise the Java GC will eat it!
    _receiver = receiver;
    setReceiverNative(receiver);
  }

	public HashMap<String, PublisherStub> getPublishers() {
		HashMap<String, PublisherStub> pubs = new HashMap<String, PublisherStub>();
		PublisherStubMap pubMap = getPublishersNative();
		StringArray pubKeys = getPubKeys();
		for (int i = 0; i < pubKeys.size(); i++) {
			pubs.put(pubKeys.get(i), pubMap.get(pubKeys.get(i)));
		}
		return pubs;
	}
%}

//******************************
// Beautify Regex class
//******************************

%rename(getSubMatchesNative) umundo::Regex::getSubMatches();
%javamethodmodifiers umundo::Regex::getSubMatches() "private";

%typemap(javaimports) umundo::Regex %{
import java.util.Vector;
%}

%typemap(javacode) umundo::Regex %{

	public Vector<String> getSubMatches() {
		Vector<String> subMatches = new Vector<String>();
		StringArray subMatchesNative = getSubMatchesNative();
		for (int i = 0; i < subMatchesNative.size(); i++) {
			subMatches.add(subMatchesNative.get(i));
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
%javamethodmodifiers umundo::Message::getKeys() "private";

%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;
%rename(getMetaNative) umundo::Message::getMeta();
%javamethodmodifiers umundo::Message::getMeta() "private";

// import java.util.HashMap
%typemap(javaimports) umundo::Message %{
import java.util.HashMap;
%}

%extend umundo::Message {
  std::vector<std::string> getMetaKeys() {
  	std::vector<std::string> keys;
  	std::map<std::string, std::string> metas = self->getMeta();
  	std::map<std::string, std::string>::iterator metaIter = metas.begin();
  	while(metaIter != metas.end()) {
  		keys.push_back(metaIter->first);
  		metaIter++;
  	}
  	return keys;
  }
};

// provide convinience methods within Message Java class for meta keys
%typemap(javacode) umundo::Message %{
public HashMap<String, String> getMeta() {
	HashMap<String, String> metas = new HashMap<String, String>();
	StringMap metaMap = getMetaNative();
	StringArray metaKeys = getMetaKeys();
	for (int i = 0; i < metaKeys.size(); i++) {
	  String key = metaKeys.get(i);
		metas.put(key, metaMap.get(key));
	}
	return metas;
}

%}

// passing a jbytearray into C++ is done by applying the STRING, LENGTH conversion above
// but we also want to return a jbytearray to get the contents of a message.
// see: http://stackoverflow.com/questions/9934059/swig-technique-to-wrap-unsigned-binary-data

%typemap(jni) char *data "jbyteArray"
%typemap(jtype) char *data "byte[]"
%typemap(jstype) char* data "byte[]"
%typemap(javaout) char* data {
  return $jnicall;
}

%typemap(out) char *data {
  $result = JCALL1(NewByteArray, jenv, ((umundo::Message const *)arg1)->size());
  JCALL4(SetByteArrayRegion, jenv, $result, 0, ((umundo::Message const *)arg1)->size(), (jbyte *)$1);
}



//******************************
// Ignore whole C++ classes
//******************************

%ignore Implementation;
%ignore Configuration;
%ignore NodeConfig;
%ignore PublisherConfig;
%ignore SubscriberConfig;
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
%ignore Mutex;
%ignore Thread;
%ignore Monitor;
%ignore MemoryBuffer;
%ignore ScopeLock;

//******************************
// Make some C++ classes package local
//******************************

//%pragma(java) moduleclassmodifiers="public final class"
%pragma(java) jniclassclassmodifiers="class"
%pragma(java) moduleclassmodifiers="class"
%typemap(javaclassmodifiers) SWIGTYPE "public class"

// not working :(
// %template(PublisherSet) std::set<umundo::Publisher>;
// %typemap(javaclassmodifiers) PublisherSet "class"
// %typemap(javaclassmodifiers) std::set<umundo::Publisher> "class"

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

%ignore Discovery(const boost::shared_ptr<DiscoveryImpl>);
%ignore Discovery(const Discovery&);

%ignore umundo::Options::getKVPs;

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

