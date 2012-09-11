import org.umundo.core.Node;
import org.umundo.protobuf.tests.EchoServiceStub;
import org.umundo.protobuf.tests.TestServices.EchoReply;
import org.umundo.protobuf.tests.TestServices.EchoRequest;
import org.umundo.rpc.ServiceDescription;
import org.umundo.rpc.ServiceFilter;
import org.umundo.rpc.ServiceManager;

public class TestRPC {

	public static void main(String[] args) throws InterruptedException {
		Node node1 = new Node();
		Node node2 = new Node();
		EchoServiceImpl localEchoService = new EchoServiceImpl();

		ServiceManager svcMgr1 = new ServiceManager();
		node1.connect(svcMgr1);
		svcMgr1.addService(localEchoService);

		ServiceManager svcMgr2 = new ServiceManager();
		node2.connect(svcMgr2);

		ServiceFilter filter = new ServiceFilter("EchoService");
		ServiceDescription svcDesc = svcMgr2.find(filter);

		EchoServiceStub echoService = new EchoServiceStub(svcDesc);

		int iterations = 15000;
	    int sends = 0;
	    long now = System.currentTimeMillis() / 1000;
	    
		while(iterations > 0) {
			EchoRequest echoReq = EchoRequest.newBuilder().setName("foo").build();
			EchoReply echoRep = echoService.echo(echoReq);
			if (echoRep.getName().compareTo("foo") != 0) {
				throw new RuntimeException("EchoReply not the same as EchoRequest");
			} else {
				sends++;
			}
			if (now < System.currentTimeMillis() / 1000) {
				System.out.println(sends + " calls per second");
				now = System.currentTimeMillis() / 1000;
				iterations--;
				sends = 0;
			}
		}
		
	}

}
