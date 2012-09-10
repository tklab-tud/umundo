/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 */

package org.umundo.rpc;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.UUID;

import org.umundo.core.Connectable;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.PublisherSet;
import org.umundo.core.Receiver;
import org.umundo.core.SubcriberSet;
import org.umundo.core.Subscriber;

public class ServiceManager extends Connectable {

	class ServiceReceiver extends Receiver {

		@Override
		public void receive(Message msg) {
			HashMap<String, String> meta = msg.getMeta();
			// is someone answering one of our requests
			if (msg.getMeta().containsKey("um.rpc.respId")) {
				String respId = msg.getMeta("um.rpc.respId");
				if (_findRequests.containsKey(respId)) {
					_findResponses.put(respId, msg);
					synchronized (_findRequests.get(respId)) {
						_findRequests.get(respId).notifyAll();
					}
					_findRequests.remove(respId);
				}
			}

			// is someone asking for a service?
			if (msg.getMeta().containsKey("um.s11n.type") && msg.getMeta("um.s11n.type").compareTo("discover") == 0) {
				ServiceFilter filter = new ServiceFilter(msg);
				for (Service svc : _svc.keySet()) {
					ServiceDescription svcDesc = _svc.get(svc);
					if (filter.matches(svcDesc)) {
						Message foundMsg = svcDesc.toMessage();
						foundMsg.putMeta("um.s11n.respId", msg.getMeta("um.s11n.reqId"));
						foundMsg.putMeta("um.rpc.channel", svc.getChannelName());
						foundMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
						_svcPub.send(foundMsg);
					}
				}
			}
		}
	}

	Map<String, Object> _findRequests = new HashMap<String, Object>();
	Map<String, Message> _findResponses = new HashMap<String, Message>();
	Map<Service, ServiceDescription> _svc = new HashMap<Service, ServiceDescription>();

	Publisher _svcPub;
	Subscriber _svcSub;
	Set<Node> _nodes = new HashSet<Node>();
	ServiceReceiver _svcReceiver = new ServiceReceiver();

	public ServiceManager() {
		_svcPub = new Publisher("umundo.sd");
		_svcSub = new Subscriber("umundo.sd", _svcReceiver);
	}

	public void addService(Service svc) {
		addService(svc, new ServiceDescription(svc.getName(), new HashMap<String, String>()));
	}

	public synchronized void addService(Service svc, ServiceDescription svcDesc) {
		_svc.put(svc, svcDesc);
		for (Node node : _nodes) {
			node.connect(svc);
		}
	}

	public synchronized void removeService(Service svc) {
		for (Node node : _nodes) {
			node.disconnect(svc);
		}
		_svc.remove(svc);
	}

	public synchronized ServiceDescription find(ServiceFilter filter) throws InterruptedException {
		Message findMsg = filter.toMessage();
		String reqId = UUID.randomUUID().toString();
		findMsg.putMeta("um.rpc.type", "discover");
		findMsg.putMeta("um.rpc.reqId", reqId);
		findMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
		_svcPub.waitForSubscribers(1);
		_svcPub.send(findMsg);

		_findRequests.put(reqId, new Object());
		synchronized (_findRequests.get(reqId)) {
			_findRequests.get(reqId).wait();
		}
		if (_findResponses.containsKey(reqId)) {
			Message foundMsg = _findResponses.get(reqId);
			ServiceDescription svcDesc = new ServiceDescription(foundMsg);
			svcDesc._svcMgr = this;
			_findResponses.remove(reqId);
			return svcDesc;
		}

		return null;
	}

	@Override
	public synchronized void addedToNode(Node node) {
		for (Service svc : _svc.keySet()) {
			node.connect(svc);
		}
		_nodes.add(node);
	}

	@Override
	public synchronized void removedFromNode(Node node) {
		if (!_nodes.contains(node))
			return;

		for (Service svc : _svc.keySet()) {
			node.disconnect(svc);
		}
		_nodes.remove(node);
	}

	@Override
	public PublisherSet getPublishers() {
		PublisherSet pubs = new PublisherSet();
		pubs.insert(_svcPub);
		return pubs;
	}

	@Override
	public SubcriberSet getSubscribers() {
		SubcriberSet subs = new SubcriberSet();
		subs.insert(_svcSub);
		return subs;
	}

	public Set<Node> getNodes() {
		return _nodes;
	}

}
