%module(directors="1", allprotected="1") umundoNativeJava
// import swig typemaps
// %include "stl_map.i"
%include <arrays_java.i>
%include <stl.i>
%include <inttypes.i>
%include "../stl_set.i"

// macros from cmake
%import "umundo/config.h"

// set UMUNDO_API macro to empty string
#define UMUNDO_API

%rename(equals) operator==; 
%rename(isValid) operator bool;

%javaconst(1);

//**************************************************
// This ends up in the generated wrapper code
//**************************************************

%{
#include "../../../../src/umundo/Host.h"
#include "../../../../src/umundo/Message.h"
#include "../../../../src/umundo/thread/Thread.h"
#include "../../../../src/umundo/Implementation.h"
#include "../../../../src/umundo/EndPoint.h"
#include "../../../../src/umundo/ResultSet.h"
#include "../../../../src/umundo/connection/PublisherStub.h"
#include "../../../../src/umundo/connection/SubscriberStub.h"
#include "../../../../src/umundo/connection/NodeStub.h"
#include "../../../../src/umundo/connection/Publisher.h"
#include "../../../../src/umundo/connection/Subscriber.h"
#include "../../../../src/umundo/connection/Node.h"
#include "../../../../src/umundo/discovery/Discovery.h"

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

#ifndef imaxabs
// google dropped imaxdiv in the android ndk r12 libc?!
intmax_t imaxabs(intmax_t j) {
	return (j < 0 ? -j : j);
}
#endif
#endif

using namespace umundo;
%}

#ifndef ANDROID
// auto loading does not work on android as the jar may not contain JNI code
%include "autoloadJNI.i"
#endif

%include "../umundo_ignores.i"
%include "../umundo_beautify.i"

//*************************************************/

// allow Java classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;


// enable conversion from char*, int to jbytearray
%apply (char *STRING, size_t LENGTH) { (const char* data, size_t length) };

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



//******************************
// Beautify Node class
//******************************

%rename(getSubscribersNative) umundo::Node::getSubscribers();
%javamethodmodifiers umundo::Node::getSubscribers() "private";
%rename(getPublishersNative) umundo::Node::getPublishers();
%javamethodmodifiers umundo::Node::getPublishers() "private";


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

// rename setter and wrap by setter in javacode typemap below
%rename(setReceiverNative) umundo::Subscriber::setReceiver(Receiver*);
%javamethodmodifiers umundo::Subscriber::setReceiver(Receiver* receiver) "private";


%typemap(javaimports) umundo::Subscriber %{
import java.util.HashMap;
%}

%typemap(javacode) umundo::Subscriber %{
  // keep receiver as a reference to prevent premature GC
  private Receiver _receiver;

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

#if 0
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
#endif

//******************************
// Beautify Message class
//******************************

%ignore umundo::Message::Message(string);
//%ignore umundo::Message::Message(const Message&);
%ignore umundo::Message::setData(const std::string&);
%ignore umundo::Message::isQueued();
%ignore umundo::Message::setQueued(bool);
%ignore umundo::Message::typeToString(uint16_t type);
%javamethodmodifiers umundo::Message::getMetaKeys() "private";

%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;
%rename(getMetaNative) umundo::Message::getMeta();
%javamethodmodifiers umundo::Message::getMeta() "private";

// messages are destroyed upon return, adopt data to Java
%typemap(javadirectorin) umundo::Message* "(msg == 0) ? null : new Message(new Message(msg, false))"

// import java.util.HashMap
%typemap(javaimports) umundo::Message %{
import java.util.HashMap;
%}

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

// GetPrimitiveArrayCritical might get us around copying
%typemap(out) char *data {
  $result = JCALL1(NewByteArray, jenv, ((umundo::Message const *)arg1)->size());
  JCALL4(SetByteArrayRegion, jenv, $result, 0, ((umundo::Message const *)arg1)->size(), (jbyte *)$1);
}

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


//***********************************************
// Parse the header file to generate wrappers
//***********************************************

%include "../../../../src/umundo/Host.h"
%include "../../../../src/umundo/Message.h"
%include "../../../../src/umundo/thread/Thread.h"
%include "../../../../src/umundo/Implementation.h"
%include "../../../../src/umundo/EndPoint.h"
%include "../../../../src/umundo/ResultSet.h"
%include "../../../../src/umundo/connection/PublisherStub.h"
%include "../../../../src/umundo/connection/SubscriberStub.h"
%include "../../../../src/umundo/connection/NodeStub.h"
%include "../../../../src/umundo/connection/Publisher.h"
%include "../../../../src/umundo/connection/Subscriber.h"
%include "../../../../src/umundo/connection/Node.h"
%include "../../../../src/umundo/discovery/Discovery.h"

