package org.umundo.samples;

import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.s11n.ITypedReceiver;
import org.umundo.s11n.TypedPublisher;
import org.umundo.s11n.TypedSubscriber;
import org.umundo.samples.SampleMessageTypes.AnotherMessage;
import org.umundo.samples.SampleMessageTypes.ChatMessage;

public class TypedPubSub {

	public static void main(String[] args) throws Exception {
		// prepare library path and load jni library
		System.loadLibrary("umundocoreSwig");
		
		// set up typed publisher (will automatically append type attribute to
		// messages)
		TypedPublisher fooPub = new TypedPublisher("fooChannel");

		// set up a typed subscriber and pass a receiver that can handle messages of type ChatMessages and AnotherMessage
		TypedSubscriber fooSub = new TypedSubscriber("fooChannel",
				new ITypedReceiver() {
					public void receiveObject(Object object, Message msg) {
						if (object instanceof ChatMessage) {
							System.out.println("chatmessage: "	+ ((ChatMessage) object).getChatline());
						} else if (object instanceof AnotherMessage) {
							System.out.println("anothermessage: "	+ ((AnotherMessage) object).getId());
						} else {
							System.out.println("other message");

						}
					}
				});
		
		// the type needs to be registered with the subscriber, so that it knows how to deserialize the messages
		fooSub.registerType(ChatMessage.class);

		Node mainNode = new Node("someDomain");
		Node otherNode = new Node("someDomain");

		mainNode.addPublisher(fooPub);
		otherNode.addSubscriber(fooSub);

		Thread.sleep(1500);
		
		for (int i = 1; i <= 64; i++) {
			ChatMessage chatMessage = ChatMessage.newBuilder().setChatline("i: " + i).build();
			fooPub.sendObject(chatMessage);
			AnotherMessage anotherMessage = AnotherMessage.newBuilder().setId(i).build();
			fooPub.sendObject(anotherMessage);
			Thread.sleep(200);
			if (i == 32) {
				//from here on try to find new deserializers by reflection
				//this should find AnotherMessage class and use it for deserialization
				fooSub.setAutoRegisterTypesByReflection(true);
			}
		}

	}
}
