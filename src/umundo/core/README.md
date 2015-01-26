# umundo.core ReadMe

## Architecture

There are two major subsystems within umundo.core:

1. The Discovery component allows you to register and query for Nodes 
within a domain. Other components can register NodeQueries
at the Discovery singleton and get notified when 
there are matching Nodes appearing or disappearing.
  
2. The Connection component with its three main classes Node, Publisher and Subscriber enables communication on channels. 
You instantiate a Node within a domain and add Publishers and Subscribers to it. The Node will register itself with the 
Discovery component and automatically hook up your Publishers and Subscribers with remote entities within the same domain.

Most of the work is being done in <a href="https://github.com/tklab-tud/umundo/blob/master/core/src/umundo/discovery/avahi/AvahiNodeDiscovery.cpp">AvahiNodeDiscovery.cpp</a> 
and <a href="https://github.com/tklab-tud/umundo/blob/master/core/src/umundo/discovery/bonjour/BonjourNodeDiscovery.cpp">BonjourNodeDiscovery.cpp</a>
for discovery and <a href="https://github.com/tklab-tud/umundo/blob/master/core/src/umundo/connection/zeromq/ZeroMQNode.cpp">ZeroMQNode.cpp</a>
for connection handling.

### Patterns

Throughout umundo.core, the Pimpl or Bridge pattern is applied - as far as I can tell, they are the same. The basic idea here 
is that all abstractions get a concrete implementation only at runtime from a Factory. This hides the concrete implementation 
from all applications and allows us to substitute the implementation without the application knowing. 

There are the following abstractions at the moment:

- [Publisher][/tklab-tud/umundo/blob/master/core/src/connection/Publisher.h]s allow you to send byte-arrays on channels.
- [Subscriber][/tklab-tud/umundo/blob/master/core/src/connection/Subscriber.h]s passes received byte arrays from channels 
to someone implementing the Receiver base class.
- [Node][/tklab-tud/umundo/blob/master/core/src/common/Node.h] is the entity where your Publishers and Receiver live and 
get connected to others in the same domain.
- [Discovery][/tklab-tud/umundo/blob/master/core/src/discovery/Discovery.h] notifies nodes of other nodes.

For every abstraction, there is a set of classes that comply to the following naming conventions:

- *Abstraction* as such is the name of the concrete client visible class. In fact this is the only thing the user is expected to 
see. It gets an implementation from a Factory singleton and has the same inheritance hierarchy as its implementation (minus the
Implementation base class). Its base classes are marked as private and it overrides *every* method to delegate it to its 
implementation.
- *AbstractionImpl* is the abstract base class for a concrete implementor. In addition to the abstraction's base classes it also
inherits Implementation. It gets inherited by concrete implementors who are registered at the Factory.
- *AbstractionConfig* is essentially unused right now but maybe used in the future to make state explicit.

### Design Decisions

During development, I made some design decisions I want to make explicit, in order not to refactor the code back into 
one of the earlier iterations:

**Why isn't every pointer a smart pointer?**
I could not get SWIG to produce readable Java bindings with smart pointers. Also, the user visible API gets somewhat convoluted
when you have to wrap everything in smart pointers. Furthermore it requires some unreadable constructs with the bridge pattern
as we cannot use our smart pointer in a constructor or destructor (i.e. when adding a Node to the Discovery).

**Why do the Abstractions override every method in their inheritance hierarchy?**
This took me a while to figure out and I am still not satisfied with the answer. When we are using abstractions and implementors,
we end up with two virtually identical inheritance hierarchies, one for the abstraction and one for the implementors (e.g. both 
the Node and the NodeImpl are EndPoints). But we want the Implementor to be able to overwrite methods even from higher up in 
the inheritance hierarchy. When the client code deals with an abstraction and sets data defined e.g. in the EndPoint class, the 
implementor won't know about it as it is the abstractions base class that gets modified. On the other hand, if the implementor 
sets its e.g. its port in its EndPoint base class, the abstraction does not know it. The solution is to make the abstraction 
class hierarchy private and delegate *every* call into the implementor. 

This has the undesired consequence that memory for all the members in the abstractions inheritance hierarchy gets allocated but 
never used. The solution would be to define all base classes without data members and define their methods abstract and pure but
then every implementing abstraction and implementor would be forced to actually implement the methods, leading to even more
boilerplate code.

**What's with the *Config classes**
Every implementation is required to overwrite Implementation::init(Configuration*) in order to get non-abstract. Originally I
planned to make all state explicit within a configuration so that we could move entities. I still think it's a good idea, but
for the moment, every implementation ignores its configuration and relies on the abstraction to call the setters for the relevant 
fields prior to calling init() with an empty configuration.

#### Things to note

**Constructors of implementors**
We are using the factory pattern for implementors, therefore the actual initialization of an implementor ought to be done in 
create(). Keep in mind that the Factory will call the standard constructor once for when creating the prototype.
