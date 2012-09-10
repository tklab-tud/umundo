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
import java.util.Map;
import java.util.UUID;

import org.umundo.core.Connectable;
import org.umundo.core.Message;
import org.umundo.core.Node;
import org.umundo.core.PublisherSet;
import org.umundo.core.SubcriberSet;
import org.umundo.s11n.ITypedReceiver;
import org.umundo.s11n.TypedPublisher;
import org.umundo.s11n.TypedSubscriber;

import com.google.protobuf.MessageLite;

public class ServiceStub extends Connectable implements ITypedReceiver {

	protected String _channelName;
	protected String _serviceName;
	protected TypedPublisher _rpcPub;
	protected TypedSubscriber _rpcSub;

	Map<String, Object> _requests = new HashMap<String, Object>();
	Map<String, Object> _responses = new HashMap<String, Object>();

	public ServiceStub(String channelName) {
		_channelName = channelName;
		_rpcPub = new TypedPublisher(_channelName);
		_rpcSub = new TypedSubscriber(_channelName, this);
	}

	public ServiceStub(ServiceDescription svcDesc) {
		_channelName = svcDesc.getChannelName();
		_serviceName = svcDesc.getName();
		_rpcPub = new TypedPublisher(_channelName);
		_rpcSub = new TypedSubscriber(_channelName, this);
		for (Node node : svcDesc._svcMgr.getNodes()) {
			node.addSubscriber(_rpcSub);
			node.addPublisher(_rpcPub);
		}
		_rpcPub.waitForSubscribers(1);
	}

	@Override
	public void receiveObject(Object object, Message msg) {
		if (msg.getMeta().containsKey("um.rpc.respId")) {
			String respId = msg.getMeta("um.rpc.respId");
			if (_requests.containsKey(respId)) {
				_responses.put(respId, object);
				synchronized (_requests.get(respId)) {
					_requests.get(respId).notifyAll();
				}
			}
		}
	}

	public Object callStubMethod(String name, MessageLite in, String inType, String outType) throws InterruptedException {
		Message rpcReqMsg = _rpcPub.prepareMessage(inType, in);
		String reqId = UUID.randomUUID().toString();
		rpcReqMsg.putMeta("um.rpc.reqId", reqId);
		rpcReqMsg.putMeta("um.rpc.method", name);
		rpcReqMsg.putMeta("um.rpc.outType", outType);
		_requests.put(reqId, new Object());
		synchronized (_requests.get(reqId)) {
			//System.out.println("Waiting");
			_rpcPub.send(rpcReqMsg);
			_requests.get(reqId).wait();
			//System.out.println("Continuing");
		}
		_requests.remove(reqId);
		Object out = _responses.get(reqId);
		_responses.remove(reqId);
		return out;
	}

	public String getChannelName() {
		return _channelName;
	}

	public String getName() {
		return _serviceName;
	}

	@Override
	public PublisherSet getPublishers() {
		PublisherSet pubs = new PublisherSet();
		pubs.insert(_rpcPub);
		return pubs;
	}

	@Override
	public SubcriberSet getSubscribers() {
		SubcriberSet subs = new SubcriberSet();
		subs.insert(_rpcSub);
		return subs;
	}

}
