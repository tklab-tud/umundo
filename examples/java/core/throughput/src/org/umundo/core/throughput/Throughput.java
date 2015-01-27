package org.umundo.core.throughput;

import java.nio.ByteBuffer;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Host;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.Receiver;
import org.umundo.core.Subscriber;
import org.umundo.core.SubscriberConfigMCast;
import org.umundo.core.SubscriberConfigRTP;

public class Throughput {

	Node node;
	Discovery disc;
	
	Publisher reporter;
	Subscriber tcpSub;
	Subscriber mcastSub;
	Subscriber rtpSub;
	ThroughputReceiver tpRcvr;
	
	Object mutex = new Object();

	long bytesRcvd = 0;
	long pktsRecvd = 0;
	long lastSeqNr = 0;
	long pktsDropped = 0;
	long currSeqNr = 0;
	long lastTimeStamp = 0;

	public class ThroughputReceiver extends Receiver {

		public void receive(Message msg) {
			synchronized (mutex) {
				bytesRcvd += msg.getSize();
				pktsRecvd++;
								
				byte[] data = msg.getData();
				ByteBuffer seqNr = ByteBuffer.wrap(data, 0, 8);
				currSeqNr = seqNr.getLong();

				ByteBuffer timeStamp = ByteBuffer.wrap(data, 8, 8);
				long currTimeStamp = timeStamp.getLong();

                ByteBuffer interval = ByteBuffer.wrap(data, 16, 4);
                int reportInterval = interval.getInt();

                if (currSeqNr < lastSeqNr)
					lastSeqNr = 0;

				if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
					pktsDropped += currSeqNr - lastSeqNr;
				}
				
				lastSeqNr = currSeqNr;
				if (currTimeStamp - reportInterval >= lastTimeStamp) {
					sendReport(currTimeStamp);
					lastTimeStamp = currTimeStamp;
				}

			}
		}
	}

	public Throughput() {
		disc = new Discovery(DiscoveryType.MDNS);
		node = new Node();

		tpRcvr = new ThroughputReceiver();
		reporter = new Publisher("reports");

		tcpSub = new Subscriber("throughput.tcp");
        tcpSub.setReceiver(tpRcvr);

		SubscriberConfigMCast mcastConfig = new SubscriberConfigMCast("throughput.mcast");
		mcastConfig.setMulticastIP("224.1.2.3");
//		mcastConfig.setMulticastPortbase(42042);
		mcastSub = new Subscriber(mcastConfig);
        mcastSub.setReceiver(tpRcvr);

		SubscriberConfigRTP rtpConfig = new SubscriberConfigRTP("throughput.rtp");
//		rtpConfig.setPortbase(40042);
		rtpSub = new Subscriber(rtpConfig);
        rtpSub.setReceiver(tpRcvr);

		disc.add(node);
		node.addSubscriber(tcpSub);
		node.addSubscriber(mcastSub);
		node.addSubscriber(rtpSub);
		node.addPublisher(reporter);

	}
	
	private void run() throws InterruptedException {
		while(true) {
			Thread.sleep(1000);
		}
	}

	public void sendReport(long currTimeStamp) {
		synchronized (mutex) {
			Message msg = new Message();

			msg.putMeta("bytes.rcvd", Long.toString(bytesRcvd));
			msg.putMeta("pkts.dropped", Long.toString(pktsDropped));
			msg.putMeta("pkts.rcvd", Long.toString(pktsRecvd));
			msg.putMeta("last.seq", Long.toString(lastSeqNr));
            msg.putMeta("last.timestamp", Long.toString(currTimeStamp));

            msg.putMeta("hostname", Host.getHostname());
            msg.putMeta("hostId", Host.getHostId());

			reporter.send(msg);
			
			pktsDropped = 0;
			pktsRecvd = 0;
			bytesRcvd = 0;
		}		
	}

	public static void main(String[] args) throws InterruptedException {
		System.load("/Users/sradomski/Documents/TK/Code/umundo/build/cli-debug/lib/libumundoNativeJava64_d.dylib");
		Throughput tp = new Throughput();
		tp.run();
	}

}
