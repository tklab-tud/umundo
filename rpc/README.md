# Remote Procedure Calls with uMundo

The umundo.rpc component contains the code necessary to invoke methods on remote
services. For now, we only support RPC with Googles ProtoBuf via protoc plugins.

## Approach

Using umundo.s11n, we already have object (de-)serialization working and can use
it together with some meta-fields to marshall and unmarshall rpc calls. At the
moment, all services in uMundo are *stateless* that is, several clients will
potentially use the same remote service object. Every service instance has a
<tt>TypedPublisher</tt> and a <tt>TypedSubscriber</tt> registered to a unique
channel. It is assumed that the individual channels are distinct or you will
run into undefined behavior at the moment (auto-generated UUIDs).
Invoking a remote method is to publish a special message on the services unique
channel and wait for a reply message in return (see message layout below).
The services channel is part of its <tt>ServiceDescription</tt>, which is acquired
by passing <tt>ServiceFilters</tt> either to <tt>ServiceManager.find()</tt> for
simple discovery, or by starting a continuous query via <tt>ServiceManager.startQuery()</tt>.

<b>Note:</b> Services may vanish at any point and with the simple <tt>ServiceManager.find()</tt>
method you may not even realize. Continuous queries provide a more robust API here.

A <tt>ServiceDescription</tt>, is simply an object, containing the services name,
its unique channel and a set of key/value pairs. You can specify the ServiceDescription
when adding a service to the ServiceManager. If you do not provide a ServiceDescription,
a default one without any key/value pairs will be created. In the C++ implementation,
you pass ServiceDescriptions to the constructor of a local stub service to get a
local representation of the remote service.

<tt>ServiceFilters</tt> are sets of (Key, Pattern, Value, Predicate) tuples and
are send when querying other ServiceManagers for services. Each tuple in a filter
is used to match the pattern against the target value for the given key from the
ServiceDescription. If the result of the match is in the given relation to the
given value, it is considered matching. If all of a filters tuples match a description,
the filter is considered to match the description and the description is returned.

1. Connect a ServiceManager instance to a Node.
<pre>
    Node* node = new Node();
    ServiceManager* svcMgr = new ServiceManager();
    node->connect(svcMgr);
</pre>
2. Add any local Services with their ServiceDescription to the ServiceManager.
<pre>
	PingService pingSvc = new PingService();
	ServiceDescription pingSvcDesc = new ServiceDescription();
	pingSvcDesc->setProperty("key", "value");
    svcMgr->addService(pingSvc, pingSvcDesc);
</pre>

3. Query the local ServiceManager for ServiceDescriptions of remote Services

	1. Either via <tt>ServiceManager.find()</tt>:
<pre>
	ServiceFilter* pingSvcFilter = new ServiceFilter("PingService");
	ServiceDescription* pingSvcDesc = svcMgr->find(pingSvcFilter);
	PingServiceStub* pingSvcStub = new PingServiceStub(pingSvcDesc);
	pingSvcStub->ping();
</pre>

	2. Or (recommended) via <tt>ServiceManager.startQuery()</tt>:
<pre>
	void added(shared_ptr<ServiceDescription> desc) {
    &nbsp;&nbsp;PingServiceStub* pingSvcStub = new PingServiceStub(pingSvcDesc);
    &nbsp;&nbsp;pingSvcStub->ping();
	}
	ServiceFilter* pingSvcFilter = new ServiceFilter("PingService");
	ServiceDescription* pingSvcDesc = svcMgr->startQuery(pingSvcFilter, this);
</pre>

To know about the Stub classes, you will need to run the umundo protoc plugin
on the <tt>.proto</tt> file. There is an example for Java in the
[build.xml](https://github.com/tklab-tud/umundo/blob/master/contrib/samples/java/rpc/chat/build.xml)
of the RPC chat sample.

## Message Layout

### RPC

<table>
	<tr><th>Meta Fields</th><th>Data</th></tr>
	<tr>
		<td>
			<tt>um.s11n.type: &lt;TYPENAME OF ARGUMENT></tt><br />
			<tt>um.rpc.reqId: &lt;UUID></tt><br />
			<tt>um.rpc.method: &lt;METHOD NAME></tt><br />
			<tt>um.rpc.outType: &lt;RETURN TYPE NAME></tt><br />
		</td>
		<td>Argument for method as a serialized object</td>
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

