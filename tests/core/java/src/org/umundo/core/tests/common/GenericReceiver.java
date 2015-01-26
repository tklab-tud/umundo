package org.umundo.core.tests.common;
import org.umundo.core.Message;
import org.umundo.core.Receiver;


public class GenericReceiver extends Receiver {

	public int bytesRcvd = 0;
	public int packetsRcvd = 0;
		
	@Override
	public void receive(Message msg) {
		bytesRcvd += msg.getSize();
		packetsRcvd++;
	}

}
