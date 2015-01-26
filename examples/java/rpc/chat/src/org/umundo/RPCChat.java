package org.umundo;


import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.Map;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Node;
import org.umundo.protobuf.ChatS11N.ChatMsg;
import org.umundo.protobuf.ChatS11N.Void;
import org.umundo.protobuf.ChatService;
import org.umundo.protobuf.ChatServiceStub;
import org.umundo.rpc.IServiceListener;
import org.umundo.rpc.ServiceDescription;
import org.umundo.rpc.ServiceFilter;
import org.umundo.rpc.ServiceManager;

/**
 * Make sure to set the correct path to umundo.jar in build.properties if you
 * want to use ant!
 */

public class RPCChat implements IServiceListener {

	/**
	 * Send and receive chat message via services. This sample uses
	 * the rpc layer to send chat messages.
	 */

	public Discovery disc;
	public Node chatNode;
	public ChatServiceImpl localChatService;
	public ServiceManager svcMgr;
	public ServiceFilter chatSvcFilter;
	public Map<ServiceDescription, ChatServiceStub> remoteChatServices = new HashMap<ServiceDescription, ChatServiceStub>();
	
	public String userName;
	public BufferedReader reader = new BufferedReader(new InputStreamReader(
			System.in));

	public RPCChat() {
		disc = new Discovery(DiscoveryType.MDNS);
		chatNode = new Node();
		disc.add(chatNode);
		
		System.out.println("Username:");
		try {
			userName = reader.readLine();
		} catch (IOException e) {
			e.printStackTrace();
		}

		svcMgr = new ServiceManager();
		localChatService = new ChatServiceImpl();
		svcMgr.addService(localChatService);		
		chatNode.connect(svcMgr);
		
		chatSvcFilter = new ServiceFilter("ChatService");
		svcMgr.startQuery(chatSvcFilter, this);

	}

	class ChatServiceImpl extends ChatService {
		@Override
		public Void receive(ChatMsg req) {
			System.out.println(req.getUsername() + ": " + req.getMessage());
			return Void.newBuilder().build();
		}

		@Override
		public Void join(ChatMsg req) {
			System.out.println(req.getUsername() + " joined the chat");
			return Void.newBuilder().build();
		}

		@Override
		public Void leave(ChatMsg req) {
			System.out.println(req.getUsername() + " left the chat");
			return Void.newBuilder().build();
		}
	}
	

	public void run() throws InterruptedException {
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
			for (ChatServiceStub remoteChatService : remoteChatServices.values()) {
				remoteChatService.receive(chatMsg);
			}
		}
		
		ChatMsg leaveMsg = ChatMsg.newBuilder().setUsername(userName).build();
		for (ChatServiceStub remoteChatService : remoteChatServices.values()) {
			remoteChatService.leave(leaveMsg);
		}

		chatNode.disconnect(svcMgr);
	}

	public static void main(String[] args) {
//		System.load("/Users/sradomski/Documents/TK/Code/umundo/build/cli/lib/libumundoNativeJava64.jnilib");
		RPCChat chat = new RPCChat();
		try {
			chat.run();
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	@Override
	public void addedService(ServiceDescription desc) {
		ChatServiceStub remoteChatSvc = new ChatServiceStub(desc);
		ChatMsg joinMsg = ChatMsg.newBuilder().setUsername(userName).build();
		try {
			remoteChatSvc.join(joinMsg);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		remoteChatServices.put(desc, remoteChatSvc);
	}

	@Override
	public void removedService(ServiceDescription desc) {
		remoteChatServices.remove(desc);
	}

	@Override
	public void changedService(ServiceDescription desc) {		
	}
}
