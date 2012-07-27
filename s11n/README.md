# Serialization with uMundo

The umundo.s11n component (s11n is to serialization as is l10n to localization) 
contains the code necessary to serialize and deserialize objects with uMundo.
For now, we only support Googles ProtoBuf.

## Approach

The general idea of using objects on top of umundo.core is pretty simple: We just 
serialize the object with an object serializer, put the byte representation as 
data in a message and publish the message. But as a subscriber you would 
have no idea that there is a serialized object in the data portion of the message, 
let alone its type. That's why we put a meta field called <tt>um.s11n.type</tt> 
with the objects classname in the message as well.

Any Receiver could just have a look at the <tt>um.s11n.type</tt> meta field and use 
the corresponding deserializer to get the object back. uMundo implements this
functionality in [<tt>TypedPublisher</tt>](https://github.com/tklab-tud/umundo/blob/master/s11n/src/umundo/s11n/TypedPublisher.h) 
and [<tt>TypedSubscriber</tt>](https://github.com/tklab-tud/umundo/blob/master/s11n/src/umundo/s11n/TypedSubscriber.h).
They inherit from <tt>Publisher</tt> and <tt>Subscriber</tt> and are pretty much 
drop-in replacements.

## Message Layout

<table>
	<caption style="caption-side: bottom; text-align: left;">Message containing an object representation</caption>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.s11n.type: &lt;ClassName></tt>
		</td>
		<td>Byte Representation of serialized object</td>
	</tr>
</table>

## Implementation Notes

At the moment, everyone needs access to the .proto files prior to compile-time, 
to generate classes for the target language. That means that applications,
will have to invoke protoc as part of their build process. 

For C++ and cmake, the UseUMUNDO.cmake module provides the <tt>UMUNDO_PROTOBUF_GENERATE_CPP_S11N</tt>
macro. For Java there is an example in the [build-java.xml](https://github.com/tklab-tud/umundo/blob/master/contrib/java/build-java.xml)
file used to build the <tt>umundocore.jar</tt>.

It is also possible to provide just the .desc files that can be created with the protoc 
compiler to get generic object representations in any target language. I am 
using this approach i.e. for the cometd JavaScript bridge as I can not know 
all the types at compile-time. But the generic object interface is rather ugly 
and should be avoided for application development.

### C++

The C++ <tt>TypedPublisher</tt> and <tt>TypedReceiver</tt> provide a
<tt>registerType(string typeName, void* serializer)</tt> method that the 
application is supposed to call with a 
[<tt>MessageLite</tt>](https://developers.google.com/protocol-buffers/docs/reference/cpp/google.protobuf.message_lite) 
 (de-)serializer prototype before usage.

### Java

The Java implementation is virtually the same approach as with C++ but without
the facade for ProtoBuf.

### Objective-C

There is a pure Objective-C implementation available, but the caveat is that
I could not find a current ProtoBuf implementation for Objective-C. As it is,
you would have to use C++ when accessing the ProtoBuf objects anyway, just 
rename your <tt>.m</tt> files to <tt>.mm</tt> to force the compiler to Objective-C++.
