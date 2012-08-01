package org.umundo.android;

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

public class UMundoAndroidActivity extends Activity {

	TextView tv;
	Thread testPublishing;
	Node node;
	Publisher fooPub;
	Subscriber fooSub;

	public class TestPublishing implements Runnable {

		@Override
		public void run() {
			String message = "This is foo from android";
			while (testPublishing != null) {
				fooPub.send(message.getBytes());
				try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					e.printStackTrace();
				}
				UMundoAndroidActivity.this.runOnUiThread(new Runnable() {
					@Override
					public void run() {
						tv.setText(tv.getText() + "o");
					}
				});
			}
		}
	}

	public class TestReceiver extends Receiver {
		@Override
		public void receive(Message msg) {

			for (String key : msg.getMeta().keySet()) {
				Log.i("umundo", key + ": " + msg.getMeta(key));
			}
			UMundoAndroidActivity.this.runOnUiThread(new Runnable() {
				@Override
				public void run() {
						tv.setText(tv.getText() + "i");
				}
			});
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

		System.loadLibrary("umundoNativeJava_d");
//		System.loadLibrary("umundoNativeJava");

		node = new Node();
		fooPub = new Publisher("pingpong");
		node.addPublisher(fooPub);
		fooSub = new Subscriber("pingpong", new TestReceiver());
		node.addSubscriber(fooSub);
		testPublishing = new Thread(new TestPublishing());
		testPublishing.start();
	}
}