package org.umundo.core.tests;

import org.umundo.core.Discovery;
import org.umundo.core.EndPoint;
import org.umundo.core.Node;
import org.umundo.core.NodeConfig;
import org.umundo.core.Publisher;
import org.umundo.core.PublisherConfigMCast;
import org.umundo.core.PublisherConfigRTP;
import org.umundo.core.PublisherConfigTCP;
import org.umundo.core.Subscriber;
import org.umundo.core.SubscriberConfigMCast;
import org.umundo.core.SubscriberConfigRTP;
import org.umundo.core.SubscriberConfigTCP;

public class TestConstructors {

	public static void main(String[] args) {
		/** Endpoint Constructors */
		{
			EndPoint ep = new EndPoint("localhost:4242");
		}

		/** Node Constructors */
		{
			Node n = new Node();
		}
		
		{
			Node n = new Node(4242, 4244);
		}

		{
			NodeConfig nodeConf = new NodeConfig();
			nodeConf.setPubPort(4244);
			nodeConf.setPort(4242);
			nodeConf.allowLocalConnections(false);
			Node n = new Node(nodeConf);
		}

		/** Discovery Constructors */
		{
			// default constructor
			Discovery disc = new Discovery(Discovery.DiscoveryType.MDNS);
		}
		{
			// default constructor
			Discovery disc = new Discovery(Discovery.DiscoveryType.BROADCAST);
		}

		/** Publisher Constructors */
		{
			Publisher pub = new Publisher("foo");
		}
		{
			PublisherConfigTCP pubConf = new PublisherConfigTCP("foo");
			Publisher pub = new Publisher(pubConf);
		}

		{
			PublisherConfigMCast pubConf = new PublisherConfigMCast("foo");
			pubConf.setPayloadType((short) 96);
			pubConf.setPortbase(42042);
			pubConf.setTimestampIncrement(166);
			Publisher pub = new Publisher(pubConf);
		}

		{
			PublisherConfigRTP pubConf = new PublisherConfigRTP("foo");
			pubConf.setPayloadType((short) 96);
			pubConf.setPortbase(42042);
			pubConf.setTimestampIncrement(166);
			Publisher pub = new Publisher(pubConf);
		}

		/** Susbcriber Constructors */
		{
			Subscriber sub = new Subscriber("foo");
		}
		{
			SubscriberConfigTCP subConf = new SubscriberConfigTCP("foo");
			Subscriber pub = new Subscriber(subConf);
		}

		{
			SubscriberConfigMCast subConf = new SubscriberConfigMCast("foo");
			subConf.setPortbase(42042);
			subConf.setMulticastIP("224.1.2.3");
			subConf.setMulticastPortbase(42142);
			Subscriber pub = new Subscriber(subConf);
		}

		{
			SubscriberConfigRTP subConf = new SubscriberConfigRTP("foo");
			subConf.setPortbase(42042);
			subConf.setMulticastIP("224.1.2.3");
			subConf.setMulticastPortbase(42142);
			Subscriber pub = new Subscriber(subConf);
		}

	}

}
