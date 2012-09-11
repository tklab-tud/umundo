import org.umundo.core.Node;
import org.umundo.rpc.ServiceDescription;
import org.umundo.rpc.ServiceManager;

public class TestRPCServer {

	public static void main(String[] args) throws InterruptedException {
		Node node = new Node();
		EchoServiceImpl localEchoService = new EchoServiceImpl();

		ServiceManager svcMgr = new ServiceManager();
		node.connect(svcMgr);
		
		  ServiceDescription echoSvcDesc = new ServiceDescription();
//		  echoSvcDesc.setProperty("host", Host.getHostId());
		  echoSvcDesc.setProperty("someString", "this is some random string with 123 numbers inside");
		  echoSvcDesc.setProperty("someNumber", "1");

		svcMgr.addService(localEchoService, echoSvcDesc);

		while(true)
			Thread.sleep(1000);
	}

}
