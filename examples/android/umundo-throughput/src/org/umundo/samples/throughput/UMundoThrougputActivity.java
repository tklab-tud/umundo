package org.umundo.samples.throughput;

import java.nio.ByteBuffer;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.RTPSubscriberConfig;
import org.umundo.core.Receiver;
import org.umundo.core.Subscriber;

import android.app.Activity;
import android.content.Context;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

/**
 * Getting it to run:
 * 
 * 1. Replace the umundo.jar in the libs folder by the one from the intaller and
 * add it to the classpath. Make sure to take the umundo.jar for Android, as you
 * are not allowed to have JNI code within and the desktop umundo.jar includes
 * all supported JNI libraries.
 * 
 * 2. Replace the JNI library libumundoNativeJava.so (or the debug variant) into
 * libs/armeabi/
 * 
 * 3. Make sure System.loadLibrary() loads the correct variant.
 * 
 * 4. Make sure you have set the correct permissions: <uses-permission
 * android:name="android.permission.INTERNET"/> <uses-permission
 * android:name="android.permission.CHANGE_WIFI_MULTICAST_STATE"/>
 * <uses-permission android:name="android.permission.ACCESS_WIFI_STATE"/>
 * 
 * @author sradomski
 * 
 */
public class UMundoThrougputActivity extends Activity {

	TextView tv;

	Thread reportPublishing;
	Discovery disc;
	Node node;
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
	
	public void sendReport(long currTimeStamp) {
		synchronized (mutex) {
			Message msg = new Message();

			msg.putMeta("bytes.rcvd", Long.toString(bytesRcvd));
			msg.putMeta("pkts.dropped", Long.toString(pktsDropped));
			msg.putMeta("pkts.rcvd", Long.toString(pktsRecvd));
			msg.putMeta("last.seq", Long.toString(lastSeqNr));
			msg.putMeta("last.timestamp", Long.toString(currTimeStamp));

			reporter.send(msg);
			
			pktsDropped = 0;
			pktsRecvd = 0;
			bytesRcvd = 0;
		}

	}
	
	public class ReportPublishing implements Runnable {

		@Override
		public void run() {
			while (reportPublishing != null) {
				UMundoThrougputActivity.this.sendReport(0);
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				UMundoThrougputActivity.this.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						// tv.setText(tv.getText() + "o");
					}
				});
			}
		}
	}

	public class ThroughputReceiver extends Receiver {
		public void receive(Message msg) {
			synchronized (mutex) {
				bytesRcvd += msg.getSize();
				pktsRecvd++;
				
				byte[] data = msg.getData();
				ByteBuffer seqNr = ByteBuffer.wrap(data, 0, 8);
				currSeqNr = seqNr.getLong();
				
				ByteBuffer timeStamp = ByteBuffer.wrap(data, 8, 16);
				long currTimeStamp = timeStamp.getLong();
				
				if (currSeqNr < lastSeqNr)
					lastSeqNr = 0;

				if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
					pktsDropped += currSeqNr - lastSeqNr;
				}
				
				lastSeqNr = currSeqNr;
				if (currTimeStamp - 1000 >= lastTimeStamp) {
					sendReport(currTimeStamp);
					lastTimeStamp = currTimeStamp;

				}

			}
		}
	}

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		tv = new TextView(this);
		tv.setText("");
		setContentView(tv);

		WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
		if (wifi != null) {
			MulticastLock mcLock = wifi.createMulticastLock("mylock");
			mcLock.acquire();
			// mcLock.release();
		} else {
			Log.v("android-umundo", "Cannot get WifiManager");
		}

		System.loadLibrary("umundoNativeJava");
		//System.loadLibrary("umundoNativeJava_d");

		disc = new Discovery(DiscoveryType.MDNS);
		node = new Node();

		tpRcvr = new ThroughputReceiver();
		reporter = new Publisher("reports");
		
		tcpSub = new Subscriber("throughput.tcp", tpRcvr);
		
		RTPSubscriberConfig mcastConfig = new RTPSubscriberConfig();
		mcastConfig.setMulticastIP("224.1.2.3");
		mcastConfig.setMulticastPortbase(42142);
		mcastSub = new Subscriber(Subscriber.SubscriberType.RTP, "throughput.mcast", tpRcvr, mcastConfig);

		RTPSubscriberConfig rtpConfig = new RTPSubscriberConfig();
		rtpConfig.setPortbase(40042);
		rtpSub = new Subscriber(Subscriber.SubscriberType.RTP, "throughput.rtp", tpRcvr, rtpConfig);

//		reportPublishing = new Thread(new ReportPublishing());
//		reportPublishing.start();
	}
	
	@Override
	protected void onPause() {
		super.onPause();
		node.removeSubscriber(tcpSub);
		node.removeSubscriber(mcastSub);
		node.removeSubscriber(rtpSub);
		node.removePublisher(reporter);
		disc.remove(node);

	}

	@Override
	protected void onResume() {
		super.onResume();
		disc.add(node);
		node.addSubscriber(tcpSub);
		node.addSubscriber(mcastSub);
		node.addSubscriber(rtpSub);
		node.addPublisher(reporter);
	}

}
