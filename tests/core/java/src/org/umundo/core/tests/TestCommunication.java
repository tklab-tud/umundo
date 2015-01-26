package org.umundo.core.tests;

import org.umundo.core.Discovery;
import org.umundo.core.Discovery.DiscoveryType;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.PublisherConfigMCast;
import org.umundo.core.PublisherConfigRTP;
import org.umundo.core.Subscriber;
import org.umundo.core.SubscriberConfigMCast;
import org.umundo.core.SubscriberConfigRTP;
import org.umundo.core.tests.common.GenericReceiver;

public class TestCommunication {
	
	public static void main(String[] args) throws InterruptedException {
		Discovery disc = new Discovery(DiscoveryType.MDNS);
		
		Node pubNode = new Node();
		Node subNode = new Node();

		disc.add(pubNode);
		disc.add(subNode);

		GenericReceiver recvTCP = new GenericReceiver(); 
		GenericReceiver recvRTP = new GenericReceiver(); 
		GenericReceiver recvMCast = new GenericReceiver(); 
		
		Publisher pubTCP = new Publisher("test.tcp");
		
		PublisherConfigRTP pubConfRTP = new PublisherConfigRTP("test.rtp");
		Publisher pubRTP = new Publisher(pubConfRTP);
		
		PublisherConfigMCast pubConfMCast = new PublisherConfigMCast("test.mcast");
		Publisher pubMCast = new Publisher(pubConfMCast);
		
		pubNode.addPublisher(pubTCP);
		pubNode.addPublisher(pubRTP);
		pubNode.addPublisher(pubMCast);
		
		Subscriber subTCP = new Subscriber("test.tcp");
		subTCP.setReceiver(recvTCP);

		SubscriberConfigRTP subConfRTP = new SubscriberConfigRTP("test.rtp"); 
		Subscriber subRTP = new Subscriber(subConfRTP);
		subRTP.setReceiver(recvRTP);

		SubscriberConfigMCast subConfMCast = new SubscriberConfigMCast("test.mcast"); 
		Subscriber subMCast = new Subscriber(subConfMCast);
		subMCast.setReceiver(recvMCast);

		subNode.addSubscriber(subTCP);
		subNode.addSubscriber(subRTP);
		subNode.addSubscriber(subMCast);
		
		byte[] data = new byte[1000];
		
		long start = System.currentTimeMillis();
		while(System.currentTimeMillis() - 5000 < start) {
			pubTCP.send(data);
			pubRTP.send(data);
			pubMCast.send(data);
			Thread.sleep(10);
		}
		
		assert(recvTCP.packetsRcvd > 0);
		assert(recvRTP.packetsRcvd > 0);
		assert(recvMCast.packetsRcvd > 0);
	}

}
