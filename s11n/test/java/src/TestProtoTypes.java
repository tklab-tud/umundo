import java.io.File;
import java.io.IOException;

import org.umundo.s11n.TypedPublisher;

import com.google.protobuf.Descriptors.DescriptorValidationException;


public class TestProtoTypes {
	/**
	 * This is not actually a real test, more a playground to check whether we can process .desc files
	 */
	public static void main(String[] args) {
		try {
			TypedPublisher.addProtoDesc(new File("types/Test.proto.desc"));
			if(TypedPublisher.protoDescForMessage("AllTypes") == null) {
				System.err.println("AllTypes was expected but not found!");
			}

//			TypedPublisher.addProtoDesc(new File("types/Dependee.proto.desc"));
			TypedPublisher.addProtoDesc(new File("types/Depender.proto.desc"));
//			TypedPublisher.addProtoDesc(new File("types"));
		} catch (IOException e) {
			e.printStackTrace();
		} catch (DescriptorValidationException e) {
			e.printStackTrace();
		}
	}

}
