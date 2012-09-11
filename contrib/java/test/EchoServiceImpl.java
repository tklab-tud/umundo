import org.umundo.protobuf.tests.EchoService;
import org.umundo.protobuf.tests.TestServices.EchoReply;
import org.umundo.protobuf.tests.TestServices.EchoRequest;

public class EchoServiceImpl extends EchoService {

	@Override
	public EchoReply echo(EchoRequest req) {
		EchoReply.Builder rep = EchoReply.newBuilder().setName(req.getName()).setBuffer(req.getBuffer());
		return rep.build();
	}
}
