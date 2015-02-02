package org.umundo.core.throughput;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
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

public class Throughput implements Runnable {

	Node node;
	Discovery disc;

	Publisher reporter;
	Subscriber tcpSub;
	Subscriber mcastSub;
	Subscriber rtpSub;
	ThroughputReceiver tpRcvr;
	public boolean isSuspended = false;

	static BufferedReader reader = new BufferedReader(new InputStreamReader(
			System.in));

	Object mutex = new Object();

	long bytesRcvd = 0;
	long pktsRecvd = 0;
	long lastSeqNr = 0;
	long pktsDropped = 0;
	long currSeqNr = 0;
	long lastTimeStamp = 0;
	long firstTimeStamp = 0;
	long startedTimeStamp = 0;

	public class ThroughputReceiver extends Receiver {

		public synchronized void receive(Message msg) {
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

				if (firstTimeStamp == 0 || currSeqNr < lastSeqNr)
					firstTimeStamp = currTimeStamp;

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

	public synchronized void suspend() {
		synchronized (mutex) {
			System.out.println("Suspending");
			node.removeSubscriber(tcpSub);
			node.removeSubscriber(mcastSub);
			node.removeSubscriber(rtpSub);
			node.removePublisher(reporter);
			disc.remove(node);
			isSuspended = true;
		}
	}

	public void resume() {
		synchronized (mutex) {
			System.out.println("Resuming");
			startedTimeStamp = System.currentTimeMillis();
			disc.add(node);
			node.addSubscriber(tcpSub);
			node.addSubscriber(mcastSub);
			node.addSubscriber(rtpSub);
			node.addPublisher(reporter);
			isSuspended = false;
		}
	}

	public Throughput() {
		disc = new Discovery(DiscoveryType.MDNS);
		node = new Node();

		tpRcvr = new ThroughputReceiver();
		reporter = new Publisher("reports");

		tcpSub = new Subscriber("throughput.tcp");
		tcpSub.setReceiver(tpRcvr);

		SubscriberConfigMCast mcastConfig = new SubscriberConfigMCast(
				"throughput.mcast");
		mcastConfig.setMulticastIP("224.1.2.3");
		// mcastConfig.setMulticastPortbase(42042);
		mcastSub = new Subscriber(mcastConfig);
		mcastSub.setReceiver(tpRcvr);

		SubscriberConfigRTP rtpConfig = new SubscriberConfigRTP(
				"throughput.rtp");
		// rtpConfig.setPortbase(40042);
		rtpSub = new Subscriber(rtpConfig);
		rtpSub.setReceiver(tpRcvr);

		startedTimeStamp = System.currentTimeMillis();
		disc.add(node);
		node.addSubscriber(tcpSub);
		node.addSubscriber(mcastSub);
		node.addSubscriber(rtpSub);
		node.addPublisher(reporter);

	}

	public void run() {
		while (true) {
			try {
				Thread.sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}

	public void sendReport(long currTimeStamp) {
		synchronized (mutex) {
			Message msg = new Message();

			msg.putMeta("pkts.rcvd", Long.toString(pktsRecvd));
			msg.putMeta("pkts.dropped", Long.toString(pktsDropped));
			msg.putMeta("bytes.rcvd", Long.toString(bytesRcvd));
			
			msg.putMeta("last.timestamp", Long.toString(currTimeStamp));
			msg.putMeta("started.timestamp", Long.toString(startedTimeStamp));
			msg.putMeta("first.timestamp", Long.toString(firstTimeStamp));
			msg.putMeta("last.seq", Long.toString(lastSeqNr));

			msg.putMeta("hostname", Host.getHostname());

			reporter.send(msg);

			pktsDropped = 0;
			pktsRecvd = 0;
			bytesRcvd = 0;
		}
	}

	public static void main(String[] args) throws InterruptedException {
//		System.load("/Users/sradomski/Documents/TK/Code/umundo/build/cli-release/lib/libumundoNativeJava64.dylib");
		System.load("/Users/sradomski/Documents/TK/Code/umundo/build/cli-debug/lib/libumundoNativeJava64_d.dylib");
		Throughput tp = new Throughput();
		Thread thread = new Thread(tp);
		thread.start();

		while (true) {
			Thread.sleep(5000);
//			if (tp.isSuspended) {
//				tp.resume();
//			} else {
//				tp.suspend();
//			}

		}
	}

}
