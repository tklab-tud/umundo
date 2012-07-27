# Remote Procedure Calls with uMundo

The umundo.rpc component contains the code necessary to invoke methods on remote
services. For now, we only support RPC with Googles ProtoBuf via protoc plugins.

## Approach

Using umundo.s11n, we already have object (de-)serialization working and can use
it together with some meta-fields to marshall and unmarshall rpc calls. At the 
moment, all services in uMundo are *stateless* that is, several instances will 
potentially use the same remote service object.

Every service instance has a <tt>TypedPublisher</tt> and a <tt>TypedSubscriber</tt>
registered to a special channel. It is assumed that the individual channels are
distinct or you will run into undefined behavior at the moment. Invoking a remote 
method is to publish a special message and wait for a reply message in return.

## Message Layout

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.ss1n.type: &lt;TYPENAME OF DATA></tt><br />
			<tt>um.rpc.reqId: &lt;UUID></tt><br />
			<tt>um.rpc.method: &lt;METHOD NAME></tt><br />
			<tt>um.rpc.outType: &lt;RETURN TYPE NAME></tt><br />
		</td>
		<td>Serialized parameter object</td>
	</tr>
	<caption style="caption-side: bottom; text-align: left;">Message representing a method invocation</caption>
</table>

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.ss1n.type: &lt;TYPENAME OF DATA></tt><br />
			<tt>um.rpc.respId: &lt;UUID OF REQUEST></tt><br />
		</td>
		<td>Serialized return object</td>
	</tr>
	<caption style="caption-side: bottom; text-align: left;">Message representing a method reply</caption>
</table>

