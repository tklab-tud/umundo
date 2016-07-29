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

%rename(equals) operator==; 
%rename(isValid) operator bool;

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

using namespace umundo;
%}

//*************************************************/

// allow Python classes to act as callbacks from C++
%feature("director") umundo::Receiver;
%feature("director") umundo::Connectable;
%feature("director") umundo::Greeter;

%include "../umundo_ignores.i"
%include "../umundo_beautify.i"

// enable conversion from char*, int to jbytearray
%apply (char *STRING, size_t LENGTH) { (const char* data, size_t length) };

// rename functions
%rename(waitSignal) wait;
%rename(yieldThis) yield;

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

