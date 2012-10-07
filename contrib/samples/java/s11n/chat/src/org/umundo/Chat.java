package org.umundo;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;

import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.protobuf.ChatS11N.ChatMsg;
import org.umundo.s11n.ITypedGreeter;
import org.umundo.s11n.ITypedReceiver;
import org.umundo.s11n.TypedPublisher;
import org.umundo.s11n.TypedSubscriber;

/**
 * Make sure to set the correct path to umundo.jar in build.properties if you
 * want to use ant!
 */

public class Chat {

	/**
	 * Send and receive serialized chat message objects. This sample uses
	 * protobuf serialization to send chat messages.
	 */

	public Node chatNode;
	public TypedSubscriber chatSub;
	public TypedPublisher chatPub;
	public ChatGreeter chatGreeter;
	public ChatReceiver chatRcv;

	public String userName;
	public HashMap<String, String> participants = new HashMap<String, String>();
	public BufferedReader reader = new BufferedReader(new InputStreamReader(
			System.in));

	public Chat() {
		chatNode = new Node();
		chatRcv = new ChatReceiver();
		chatSub = new TypedSubscriber("s11nChat", chatRcv);
		chatPub = new TypedPublisher("s11nChat");
		chatSub.registerType(ChatMsg.class);

		System.out.println("Username:");
		try {
			userName = reader.readLine();
		} catch (IOException e) {
			e.printStackTrace();
		}

		ChatGreeter greeter = new ChatGreeter();
		greeter.userName = userName;
		chatPub.setGreeter(greeter);

		chatNode.addPublisher(chatPub);
		chatNode.addSubscriber(chatSub);
	}

	class ChatReceiver implements ITypedReceiver {

		@Override
		public void receiveObject(Object object, Message msg) {
			if (object != null) {
				ChatMsg chatMsg = (ChatMsg) object;

				if (chatMsg.getType() == ChatMsg.Type.JOINED) {
					Chat.this.participants.put(msg.getMeta("subscriber"),
							chatMsg.getUsername());
					System.out.println(chatMsg.getUsername()
							+ " joined the chat");
				} else if (chatMsg.getType() == ChatMsg.Type.NORMAL) {
					System.out.println(chatMsg.getUsername() + ": "
							+ chatMsg.getMessage());
				}
			}
		}
	}

	class ChatGreeter implements ITypedGreeter {
		public String userName;

		@Override
		public void welcome(TypedPublisher pub, String nodeId, String subId) {
			ChatMsg welcomeMsg = ChatMsg.newBuilder().setUsername(userName)
					.setType(ChatMsg.Type.JOINED).build();
			Message greeting = pub.prepareMessage("ChatMsg", welcomeMsg);
			greeting.setReceiver(subId);
			greeting.putMeta("subscriber", Chat.this.chatSub.getUUID());
			pub.send(greeting);
		}

		@Override
		public void farewell(TypedPublisher pub, String nodeId, String subId) {
			if (Chat.this.participants.containsKey(subId)) {
				System.out.println(Chat.this.participants.get(subId)
						+ " left the chat");
			} else {
				System.out.println("An unknown user left the chat: " + subId);
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

			ChatMsg chatMsg = ChatMsg.newBuilder().setUsername(userName)
					.setMessage(line).build();
			chatPub.sendObject("ChatMsg", chatMsg);
		}
		chatNode.removePublisher(chatPub);
		chatNode.removeSubscriber(chatSub);
	}

	public static void main(String[] args) {
		Chat chat = new Chat();
		chat.run();
	}
}
