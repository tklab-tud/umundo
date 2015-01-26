import org.umundo.core.Discovery;
import org.umundo.core.Node;
import org.umundo.protobuf.tests.EchoServiceStub;
import org.umundo.protobuf.tests.TestServices.EchoReply;
import org.umundo.protobuf.tests.TestServices.EchoRequest;
import org.umundo.rpc.IServiceListener;
import org.umundo.rpc.ServiceDescription;
import org.umundo.rpc.ServiceFilter;
import org.umundo.rpc.ServiceManager;

public class RPCClient {
	public static void main(String[] args) throws InterruptedException {
		class ServiceListener implements IServiceListener {
			@Override
			public void addedService(ServiceDescription desc) {
				System.out.println("Found Service");
//				try {
//					EchoServiceStub echoSvc = new EchoServiceStub(desc);
//					EchoRequest echoReq = EchoRequest.newBuilder().setName("foo").build();
//					EchoReply echoRep;
//					echoRep = echoSvc.echo(echoReq);
//					if (echoRep.getName().compareTo("foo") != 0) {
//						throw new RuntimeException("EchoReply not the same as EchoRequest");
//					}			
//				} catch (InterruptedException e) {
//					e.printStackTrace();
//				}
			}

			@Override
			public void removedService(ServiceDescription desc) {
				System.out.println("Lost Service");
			}

			@Override
			public void changedService(ServiceDescription desc) {
				System.out.println("Changed Service");
			}
		}

		Discovery disc = new Discovery(Discovery.DiscoveryType.MDNS);
		Node node = new Node();
		disc.add(node);
		
		ServiceManager svcMgr = new ServiceManager();
		node.connect(svcMgr);
		
		ServiceFilter echoFilter = new ServiceFilter("EchoService");
		ServiceDescription echoSvcDesc = svcMgr.find(echoFilter);

		if (echoSvcDesc != null) {
			EchoServiceStub echoSvc = new EchoServiceStub(echoSvcDesc);
			int iterations = 1000;
			while(iterations-- > 0) {
				EchoRequest echoReq = EchoRequest.newBuilder().setName("foo").build();
				EchoReply echoRep = echoSvc.echo(echoReq);
				if (echoRep.getName().compareTo("foo") != 0) {
					throw new RuntimeException("EchoReply not the same as EchoRequest");
				}
				System.out.print(".");
			}
			
		} else {
			System.err.println("Could not find EchoService");
		}
		
		ServiceListener svcListener = new ServiceListener();
		svcMgr.startQuery(echoFilter, svcListener);
		
		while(true)
			Thread.sleep(1000);
		
	}

}
