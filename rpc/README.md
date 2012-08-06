# Remote Procedure Calls with uMundo

The umundo.rpc component contains the code necessary to invoke methods on remote
services. For now, we only support RPC with Googles ProtoBuf via protoc plugins.

## Approach

Using umundo.s11n, we already have object (de-)serialization working and can use
it together with some meta-fields to marshall and unmarshall rpc calls. At the
moment, all services in uMundo are *stateless* that is, several instances will
potentially use the same remote service object.

Every service instance has a <tt>TypedPublisher</tt> and a <tt>TypedSubscriber</tt>
registered to a unique channel. It is assumed that the individual channels are
distinct or you will run into undefined behavior at the moment. Invoking a remote
method is to publish a special message and wait for a reply message in return.

Every service is furthermore described via its <tt>ServiceDescription</tt>, a simple
object, containing the service name, its unique channel and a set of key/value pairs.
You can specify the ServiceDescription when adding a service to the ServiceManager.
If you do not provide a ServiceDescription, a default one without any key/value pairs
will be created. To actually invoke services, you need to pass their ServiceDescription
to the constructor of a local stub service.

ServiceDescriptions are acquired by passing <tt>ServiceFilters</tt> either to
<tt>ServiceManager.find()</tt>, which will return the first matching ServiceDescription,
or by starting a continuous query via <tt>ServiceManager.startQuery()</tt> and
waiting for results.

<tt>ServiceFilters</tt> are a set of (Key, Pattern, Value, Predicate) tuples. Each
tuple is used to match the given pattern on the  ServiceDescriptions

## Message Layout

### Service Query


#### Simple Queries

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.rpc.type: 'discover'</tt><br />
			<tt>um.rpc.reqId: &lt;UUID of request></tt><br />
			<tt>um.rpc.mgrId: &lt;UUID of originating service manager></tt><br /><br />
			<tt>um.rpc.filter.svcName: &lt;name of service to find></tt><br />
			<tt>um.rpc.filter.uuid: &lt;UUID of filter></tt> (unused with simple queries)<br /><br />
			<tt>um.rpc.filter.value.ATTR_NAME: &lt;required value for a service description attribute></tt><br />
			<tt>um.rpc.filter.pattern.ATTR_NAME: &lt;pattern to match substring on attribute></tt><br />
			<tt>um.rpc.filter.pred.ATTR_NAME: &lt;predicate for match to value></tt><br />
		</td>
		<td>Empty</td>
	</tr>
	<caption style="caption-side: bottom; text-align: left;">Message to find a service sent by ServiceManager.find()</caption>
</table>


#### Continuous Queries

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.rpc.type: 'startDiscovery' / 'stopDiscovery'</tt><br />
			<tt>um.rpc.mgrId: &lt;UUID of originating service manager></tt><br /><br />
			<tt>um.rpc.filter.svcName: &lt;name of service to find></tt><br />
			<tt>um.rpc.filter.uuid: &lt;UUID of filter></tt><br /><br />
			<tt>um.rpc.filter.value.ATTR_NAME: &lt;value required for a service description attribute></tt><br />
			<tt>um.rpc.filter.pattern.ATTR_NAME: &lt;pattern to match substring on attribute></tt><br />
			<tt>um.rpc.filter.pred.ATTR_NAME: &lt;predicate for match to value></tt><br />
		</td>
		<td>Empty</td>
	</tr>
	<caption style="caption-side: bottom; text-align: left;">Message for continuous queries via ServiceManager.[start|stop]Query</caption>
</table>

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.rpc.type: 'startDiscovery' / 'stopDiscovery'</tt><br />
			<tt>um.rpc.mgrId: &lt;UUID of originating service manager></tt><br /><br />
			<tt>um.rpc.filter.svcName: &lt;name of service to find></tt><br />
			<tt>um.rpc.filter.uuid: &lt;UUID of filter></tt><br /><br />
			<tt>um.rpc.filter.value.ATTR_NAME: &lt;required value for a service description attribute></tt><br />
			<tt>um.rpc.filter.pattern.ATTR_NAME: &lt;pattern to match substring on attribute></tt><br />
			<tt>um.rpc.filter.pred.ATTR_NAME: &lt;predicate for match to value></tt><br />
		</td>
		<td>Empty</td>
	</tr>
	<caption style="caption-side: bottom; text-align: left;">Message for continuous queries via ServiceManager.[start|stop]Query</caption>
</table>

### Simple RPC

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.s11n.type: &lt;TYPENAME OF DATA></tt><br />
			<tt>um.rpc.reqId: &lt;UUID></tt><br />
			<tt>um.rpc.method: &lt;METHOD NAME></tt><br />
			<tt>um.rpc.outType: &lt;RETURN TYPE NAME></tt><br />
		</td>
		<td>Parameter for called method as serialized object</td>
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

