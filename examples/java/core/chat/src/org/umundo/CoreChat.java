package org.umundo;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Greeter;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.Receiver;
import org.umundo.core.Subscriber;
import org.umundo.core.SubscriberStub;

/**
 * Make sure to set the correct path to umundo.jar in build.properties if you want to use ant!
 */

public class CoreChat {

	/**
	 * Send and receive simple chat messages. This sample uses meta fields from
	 * a message to send chat strings.
	 */
	
	public Discovery disc;
	public Node chatNode;
	public Subscriber chatSub;
	public Publisher chatPub;
	
	public String userName;
	public HashMap<String, String> participants = new HashMap<String, String>();
	public BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));

	public CoreChat() {
		disc = new Discovery(DiscoveryType.MDNS);
		
		chatNode = new Node();
		chatSub = new Subscriber("coreChat");
		chatSub.setReceiver(new ChatReceiver());
		
		chatPub = new Publisher("coreChat");
		
		disc.add(chatNode);
		
		System.out.println("Username:");
		try {
			userName = reader.readLine();
		} catch (IOException e) {
			e.printStackTrace();
		}
		System.gc();
		
		chatPub.setGreeter(new ChatGreeter(userName));
		chatNode.addPublisher(chatPub);
		chatNode.addSubscriber(chatSub);
	}
	
	class ChatReceiver extends Receiver {

		@Override
		public void receive(Message msg) {
			if (msg.getMeta().containsKey("participant")) {
				CoreChat.this.participants.put(msg.getMeta("subscriber"), msg.getMeta("participant"));
				System.out.println(msg.getMeta("participant") + " joined the chat");
			} else {
				System.out.println(msg.getMeta("userName") + ": "
						+ msg.getMeta("chatMsg"));
			}
//			System.out.println(msg.getMeta());
		}
	}

	class ChatGreeter extends Greeter {
		public String userName;

		public ChatGreeter(String userName) {
			this.userName = userName;
		}

		@Override
		public void welcome(Publisher pub, SubscriberStub subStub) {
			Message greeting = Message.toSubscriber(subStub.getUUID());
			greeting.putMeta("participant", userName);
			greeting.putMeta("subscriber", CoreChat.this.chatSub.getUUID());
			pub.send(greeting);
		}

		@Override
		public void farewell(Publisher pub, SubscriberStub subStub) {
			if (CoreChat.this.participants.containsKey(subStub.getUUID())) {
				System.out.println(CoreChat.this.participants.get(subStub.getUUID()) + " left the chat");
			} else {
				System.out.println("An unknown user left the chat: " + subStub.getUUID());	
			}
		}	
	}
	
	public void run() {
		System.out.println("Start typing messages (empty line to quit):");
		while (true) {
			String line = "";
			try {
				line = reader.readLine();
			} catch (IOException e) {
				e.printStackTrace();
			}
			if (line.length() == 0)
				break;
			Message msg = new Message();
			msg.putMeta("userName", userName);
			msg.putMeta("chatMsg", line);
			chatPub.send(msg);
		}
		chatNode.removePublisher(chatPub);
		chatNode.removeSubscriber(chatSub);
	}
	
	public static void main(String[] args) {
//		System.load("/Users/sradomski/Documents/TK/Code/umundo/build/cli/lib/libumundoNativeJava64.jnilib");
		CoreChat chat = new CoreChat();
		chat.run();
	}
}

