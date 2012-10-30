%module(directors="1", allprotected="1") umundoNativeCSharp
// import swig typemaps
%include <arrays_csharp.i>
%include <stl.i>
%include <inttypes.i>
//%include "boost_shared_ptr.i"

// macros from cmake
%import "umundo/config.h"

// set DLLEXPORT macro to empty string
#define DLLEXPORT

// SWIG does not recognize 'using std::string' from an include
typedef std::string string;
typedef std::vector vector;
typedef std::set set;

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
%ignore umundo::Message::Message(string);
%rename(getData) umundo::Message::data;
%rename(getSize) umundo::Message::size;


//******************************
// Lets hope this will work some day
//******************************
# %typemap(cstype)  (char *STRING, size_t LENGTH) "byte[]"

# %typemap(ctype)   (char *STRING, size_t LENGTH) "byte[]"
# %typemap(imtype)  (char *STRING, size_t LENGTH) (IntPtr, size_t)
# %typemap(csin)  (char *STRING, size_t LENGTH) "wergwerg"

# %typemap(in)      (char *STRING, size_t LENGTH) {
# 	$1 = pointer($input);
# }

# %typemap(in)      (char *STRING, size_t LENGTH) {
# 	$1 = pointer($input);
# 	$2 = pointer($input);
# 	$3 = pointer($input);
# 	$4 = size($input);
# }

# %typemap(csin, pre="    _receiver = $csinput;") (char *STRING, size_t LENGTH) "asdf"

# %typemap(csvarin)      (char *STRING, size_t LENGTH) {
#     typemap csvarin
# }


# %typemap(cstype)  (char *STRING, size_t LENGTH) "byte[]"
# %typemap(ctype)   (char *STRING, size_t LENGTH) "byte[]"
# %typemap(imtype)  (char *STRING, size_t LENGTH) "IntPtr"
# %typemap(in)      (char *STRING, size_t LENGTH) {
#     $1 = (char *) JCALL2(GetByteArrayElements, jenv, $input, 0);
#     $2 = (size_t) JCALL1(GetArrayLength,       jenv, $input);
# }

# %typemap(freearg) (char *STRING, size_t LENGTH) ""
# %typemap(argout)  (char *STRING, size_t LENGTH) {
#     JCALL3(ReleaseByteArrayElements, jenv, $input, (jbyte *)$1, 0);
# }

// enable conversion from char*, int to byte[]
# %apply (char *STRING, int LENGTH) { (const char* data, size_t length) };


# %typemap(cstype) char *data "byte[]" 
# %typemap(imtype) char *data "IntPtr" 
# %typemap(csout) char *data %{
# 	{
#     byte[] ret = new byte[this.getSize()]; 
#     IntPtr data = $imcall; 
#     System.Runtime.InteropServices.Marshal.Copy(data, ret, 0, this.getSize());
#     return ret; 
#   } 
# %} 

# %typemap(out) char *data {
#   $result = JCALL1(NewByteArray, jenv, ((umundo::Message const *)arg1)->size());
#   JCALL4(SetByteArrayRegion, jenv, $result, 0, ((umundo::Message const *)arg1)->size(), (jbyte *)$1);
# }

//******************************
//******************************


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
%include "../../../../core/src/umundo/connection/Publisher.h"
%include "../../../../core/src/umundo/connection/Subscriber.h"
%include "../../../../core/src/umundo/connection/Node.h"

