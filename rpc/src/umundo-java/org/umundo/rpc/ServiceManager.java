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
import java.util.Vector;

import org.umundo.core.Connectable;
import org.umundo.core.Greeter;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.Publisher;
import org.umundo.core.PublisherSet;
import org.umundo.core.Receiver;
import org.umundo.core.SubscriberSet;
import org.umundo.core.Subscriber;

public class ServiceManager extends Connectable {

	class ServiceReceiver extends Receiver {

		@Override
		public void receive(Message msg) {
			HashMap<String, String> meta = msg.getMeta();
			// is this a response for one of our requests?
			if (msg.getMeta().containsKey("um.rpc.respId")) {
				String respId = msg.getMeta("um.rpc.respId");
				if (_findRequests.containsKey(respId)) {
					_findResponses.put(respId, msg);
					synchronized (_findRequests.get(respId)) {
						_findRequests.get(respId).notifyAll();
					}
				}
			}

			// is someone simply asking for a service via find?
			if (msg.getMeta().containsKey("um.rpc.type") && msg.getMeta("um.rpc.type").compareTo("discover") == 0) {
				ServiceFilter filter = new ServiceFilter(msg);
				Vector<ServiceDescription> foundSvcs = findLocal(filter);
				
				if (!foundSvcs.isEmpty()) {
					for (ServiceDescription desc : foundSvcs) {
						Message foundMsg = desc.toMessage();
						foundMsg.setReceiver(msg.getMeta("um.rpc.mgrId"));
						foundMsg.putMeta("um.rpc.respId", msg.getMeta("um.rpc.reqId"));
						foundMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
						
						if (_svcPub.isPublishingTo(msg.getMeta("um.rpc.mgrId"))) {
							_svcPub.send(foundMsg);
						} else {
							if (!_pendingMessages.containsKey(msg.getMeta("um.rpc.mgrId"))) {
								_pendingMessages.put(msg.getMeta("um.rpc.mgrId"), new Vector<Message>());
							}
							_pendingMessages.get(msg.getMeta("um.rpc.mgrId")).add(foundMsg);
						}
					}
				}
			}

			// is this the start of a continuous query?
			if (msg.getMeta().containsKey("um.rpc.type") && msg.getMeta("um.rpc.type").compareTo("startDiscovery") == 0) {
				ServiceFilter filter = new ServiceFilter(msg);
				_remoteQueries.put(filter._uuid, filter);

				// do we have such a service?
				Vector<ServiceDescription> foundSvcs = findLocal(filter);
				for (ServiceDescription desc : foundSvcs) {
					Message foundMsg = desc.toMessage();
					foundMsg.setReceiver(msg.getMeta("um.rpc.mgrId"));
					foundMsg.putMeta("um.rpc.filterId", filter.getUUID());
					foundMsg.putMeta("um.rpc.type", "discovered");
					foundMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
					_svcPub.send(foundMsg);
				}
			}
			
			// is this the end of a continuous query?
			if (msg.getMeta().containsKey("um.rpc.type") && 
					(msg.getMeta("um.rpc.type").compareTo("discovered") == 0 || msg.getMeta("um.rpc.type").compareTo("vanished") == 0)) {
				ServiceFilter filter = null;
				for (ServiceFilter currFilter : _localQueries.keySet()) {
					if (currFilter.getUUID().compareTo(msg.getMeta("um.rpc.filterId")) == 0) {
						filter = currFilter;
					}
				} 
				if (filter != null) {
					IServiceListener listener = _localQueries.get(filter);
					String svcChannel = msg.getMeta("um.rpc.desc.channel");
					String managerId = msg.getMeta("um.rpc.mgrId");
					if (!_remoteSvcDesc.containsKey(managerId) || !_remoteSvcDesc.get(managerId).containsKey(svcChannel)) {
						_remoteSvcDesc.put(managerId, new HashMap<String, ServiceDescription>());
						_remoteSvcDesc.get(managerId).put(svcChannel, new ServiceDescription(msg));
						_remoteSvcDesc.get(managerId).get(svcChannel)._svcMgr = ServiceManager.this;
					}
					if (msg.getMeta("um.rpc.type").compareTo("discovered") == 0) {
						listener.addedService(_remoteSvcDesc.get(managerId).get(svcChannel));
					} else {
						listener.removedService(_remoteSvcDesc.get(managerId).get(svcChannel));
						_remoteSvcDesc.get(managerId).remove(svcChannel);
					}
				}
			}

			// is this a reply to a continuous service query?
			if (msg.getMeta().containsKey("um.rpc.type") && msg.getMeta("um.rpc.type").compareTo("stopDiscovery") == 0) {
			}
		}
	}

	class ServiceGreeter extends Greeter {
		  public void welcome(Publisher arg0, String nodeId, String subId) {
			  for (ServiceFilter filter : _localQueries.keySet()) {
				  Message queryMsg = filter.toMessage();
				  queryMsg.setReceiver(subId);
				  queryMsg.putMeta("um.rpc.type", "startDiscovery");
				  queryMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
				  _svcPub.send(queryMsg);
			  }
			  if (_pendingMessages.containsKey(subId)) {
				  for (Message msg : _pendingMessages.get(subId)) {
					  _svcPub.send(msg);
				  }
				  _pendingMessages.remove(subId);
			  }
		  }
		  
		  public void farewell(Publisher arg0, String nodeId, String subId) {
			  if (_remoteSvcDesc.containsKey(subId)) {
				  for (ServiceFilter filter : _localQueries.keySet()) {
					  for (String svcChannel : _remoteSvcDesc.get(subId).keySet()) {
						  if (filter.matches(_remoteSvcDesc.get(subId).get(svcChannel))) {
							  _localQueries.get(filter).removedService(_remoteSvcDesc.get(subId).get(svcChannel));
						  }
					  }
				  }
				  _remoteSvcDesc.remove(subId);
			  }
		  }
	}
	
	Map<String, Object> _findRequests = new HashMap<String, Object>();
	Map<String, Message> _findResponses = new HashMap<String, Message>();
	Map<Service, ServiceDescription> _svc = new HashMap<Service, ServiceDescription>();
	Map<ServiceFilter, IServiceListener> _localQueries = new HashMap<ServiceFilter, IServiceListener>();
	Map<String, Vector<Message>> _pendingMessages = new HashMap<String, Vector<Message>>();
	Map<String, ServiceFilter> _remoteQueries = new HashMap<String, ServiceFilter>();
	Map<String, Map<String, ServiceDescription > > _remoteSvcDesc = new HashMap<String, Map<String, ServiceDescription > >(); ///< Remote mgrIds to channel names to descriptions of remote services

	Publisher _svcPub;
	Subscriber _svcSub;
	Set<Node> _nodes = new HashSet<Node>();
	ServiceReceiver _svcReceiver = new ServiceReceiver();
	ServiceGreeter _svcGreeter = new ServiceGreeter();

	public ServiceManager() {
		_svcPub = new Publisher("umundo.sd");
		_svcSub = new Subscriber("umundo.sd", _svcReceiver);
		_svcPub.setGreeter(_svcGreeter);
	}

	public void addService(Service svc) {
		addService(svc, new ServiceDescription(new HashMap<String, String>()));
	}

	public synchronized void addService(Service svc, ServiceDescription desc) {
		desc.setChannelName(svc.getChannelName());
		desc.setServiceName(svc.getName());
		_svc.put(svc, desc);
		for (Node node : _nodes) {
			node.connect(svc);
		}
		
		// iterate continuous queries and notify other service managers about matches.
		for (ServiceFilter filter : _remoteQueries.values()) {
			if (filter.matches(desc)) {
				Message foundMsg = desc.toMessage();
				foundMsg.putMeta("um.rpc.filterId", filter.getUUID());
				foundMsg.putMeta("um.rpc.type", "discovered");
				foundMsg.putMeta("um.rpc.channel", desc.getChannelName());
				foundMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
				_svcPub.send(foundMsg);

			}
		}
	}

	public synchronized void removeService(Service svc) {
		for (Node node : _nodes) {
			node.disconnect(svc);
		}
		
		ServiceDescription desc = _svc.get(svc);

		// iterate continuous queries and notify other service managers about removals.
		for (ServiceFilter filter : _remoteQueries.values()) {
			if (filter.matches(desc)) {
				Message foundMsg = desc.toMessage();
				foundMsg.putMeta("um.rpc.filterId", filter.getUUID());
				foundMsg.putMeta("um.rpc.type", "vanished");
				foundMsg.putMeta("um.rpc.channel", desc.getChannelName());
				foundMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
				_svcPub.send(foundMsg);
			}
		}
		_svc.remove(svc);
	}

	public void startQuery(ServiceFilter filter, IServiceListener listener) {
		_localQueries.put(filter, listener);
		Message queryMsg = filter.toMessage();
		queryMsg.putMeta("um.rpc.type", "startDiscovery");
		queryMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
		_svcPub.send(queryMsg);
	}
	
	public void stopQuery(ServiceFilter filter) {
		if (_localQueries.containsKey(filter)) {
			Message unqueryMsg = filter.toMessage();
			unqueryMsg.putMeta("um.rpc.type", "stopDiscovery");
			unqueryMsg.putMeta("um.rpc.mgrId", _svcSub.getUUID());
			_svcPub.send(unqueryMsg);
			_localQueries.remove(filter);
		}
	}

	Vector<ServiceDescription> findLocal(ServiceFilter svcFilter) {
		Vector<ServiceDescription> foundSvcs = new Vector<ServiceDescription>();
		for(ServiceDescription desc : _svc.values()) {
			if (svcFilter.matches(desc)) {
				foundSvcs.add(desc);
			}
		}
		return foundSvcs;
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
			_findRequests.get(reqId).wait(3000);
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
	public SubscriberSet getSubscribers() {
		SubscriberSet subs = new SubscriberSet();
		subs.insert(_svcSub);
		return subs;
	}

	public Set<Node> getNodes() {
		return _nodes;
	}

}
