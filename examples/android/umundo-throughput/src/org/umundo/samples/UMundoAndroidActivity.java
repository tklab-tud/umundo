package org.umundo.samples;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
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
public class UMundoAndroidActivity extends Activity {

	TextView tv;

	Thread reportPublishing;
	Discovery disc;
	Node node;
	Publisher pub;
	Subscriber sub;

	Object mutex = new Object();

	long bytesRcvd = 0;
	long pktsRecvd = 0;
	long lastSeqNr = 0;
	long pktsDropped = 0;

	public class ReportPublishing implements Runnable {

		@Override
		public void run() {
			while (reportPublishing != null) {
				synchronized (mutex) {
					Message msg = new Message();

					msg.putMeta("bytes.rcvd", Long.toString(bytesRcvd));
					msg.putMeta("pkts.dropped", Long.toString(pktsDropped));
					msg.putMeta("pkts.rcvd", Long.toString(pktsRecvd));
					msg.putMeta("last.seq", Long.toString(lastSeqNr));

					pub.send(msg);
					
					pktsDropped = 0;
					pktsRecvd = 0;
					bytesRcvd = 0;
				}
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				UMundoAndroidActivity.this.runOnUiThread(new Runnable() {
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
				long currSeqNr = Long.parseLong(msg.getMeta("seqNr"));
				
				if (currSeqNr < lastSeqNr)
					lastSeqNr = 0;

				if (lastSeqNr > 0 && lastSeqNr != currSeqNr - 1) {
					pktsDropped += currSeqNr - lastSeqNr;
				}
				
				lastSeqNr = currSeqNr;
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

		// System.loadLibrary("umundoNativeJava");
		System.loadLibrary("umundoNativeJava_d");

		disc = new Discovery(DiscoveryType.MDNS);

		node = new Node();
		disc.add(node);

		pub = new Publisher("reports");
		node.addPublisher(pub);

		sub = new Subscriber("throughput", new ThroughputReceiver());
		node.addSubscriber(sub);

		reportPublishing = new Thread(new ReportPublishing());
		reportPublishing.start();
	}
}
