%module(directors="1", allprotected="1") umundoNativeJava
// import swig typemaps
%include <arrays_java.i>
%include <stl.i>
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
typedef std::list list;

%javaconst(1);

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

#if 0
jint JNI_OnLoad(JavaVM *vm, void *reserved) {
	using umundo::Debug;
//	LOG_ERR("This is mundo.core speaking!");
	return JNI_VERSION_1_2;
}
#endif

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

using std::string;
using std::vector;
using std::map;
using boost::shared_ptr;
using namespace umundo;
%}

#ifndef ANDROID
// auto loading does not work on android as the jar may not contain JNI code
%include "autoloadJNI.i"
#endif

//*************************************************/

// Provide a nicer Java interface to STL containers
%template(StringVector) std::vector<std::string>;
%template(StringSet)    std::set<std::string>;
%template(PublisherSet) std::set<umundo::Publisher*>;
%template(SubcriberSet) std::set<umundo::Subscriber*>;

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

# # hold a reference to the receiver in the subscriber
# %ignore umundo::Subscriber::Subscriber(string, Receiver*);
# %typemap(javacode) umundo::Subscriber %{
#   private Receiver _receiver;
#
#   public Subscriber(String channelName, Receiver receiver) {
#     this(umundoNativeJavaJNI.new_Subscriber(channelName), true);
# 		setReceiver(receiver);
#   }
#
#   protected void setReceiver(Receiver receiver) {
# 		_receiver = receiver;
#     umundoNativeJavaJNI.Subscriber_setReceiver(swigCPtr, this, Receiver.getCPtr(receiver), receiver);
#   }
# %}
# //%typemap(javain, pre="    _receiver = $javainput;") umundo::Receiver* "$javainput"

# messages are destroyed upon return, always pass copies to Java
%typemap(javadirectorin) umundo::Message* "(msg == 0) ? null : new Message(new Message(msg, false))"

//******************************
// Beautify Node class
//******************************

%ignore umundo::Node::hasSubscriber(const string& uuid);
%ignore umundo::Node::getSubscriber(const string& uuid);
%ignore umundo::Node::getSubscribers();

%ignore umundo::Node::hasPublisher(const string& uuid);
%ignore umundo::Node::getPublisher(const string& uuid);
%ignore umundo::Node::getPublishers();

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
		StringVector subMatchesNative = getSubMatchesNative();
		for (int i = 0; i < subMatchesNative.size(); i++) {
			subMatches.add(subMatchesNative.get(i));
		}
		return subMatches;
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
%ignore umundo::Message::Message(string);
%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;

// import java.util.HashMap
%typemap(javaimports) umundo::Message %{
import java.util.HashMap;
%}

#if 0
	public Message(Message other) {
		this(); // call actual constructor from SWIG
		setData(other.getData());
		HashMap<String, String> meta = other.getMeta();
		if (meta != null)
			for (String k : meta.keySet()) {
				putMeta(k, meta.get(k));
			}
	}
#endif

// provide convinience methods within Message Java class for meta keys
%typemap(javacode) umundo::Message %{

	public HashMap<String, String> getMeta() {
		HashMap<String, String> keys = new HashMap<String, String>();
		for (int i = 0; i < getKeys().size(); i++) {
			keys.put(getKeys().get(i), getMeta(getKeys().get(i)));
		}
		return keys;
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
// Beautify Publisher class
//******************************

%javamethodmodifiers umundo::Subscriber::Subscriber(string channelName) "protected";
%javamethodmodifiers umundo::Subscriber::setReceiver(Receiver* receiver) "protected";


//******************************
// Ignore whole C++ classes
//******************************

%ignore Implementation;
%ignore Configuration;
%ignore NodeConfig;
%ignore PublisherConfig;
%ignore SubscriberConfig;
%ignore NodeImpl;
%ignore PublisherImpl;
%ignore SubscriberImpl;
%ignore Mutex;
%ignore Thread;
%ignore Monitor;
%ignore MemoryBuffer;
%ignore ScopeLock;


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

