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

import java.util.UUID;

import org.umundo.core.Message;
import org.umundo.s11n.ITypedReceiver;

import com.google.protobuf.MessageLite;

public abstract class Service extends ServiceStub implements ITypedReceiver {

	public Service() {
		this(UUID.randomUUID().toString());
	}
	
	private Service(ServiceDescription svcDesc) {
		super(svcDesc);
	}

	private Service(String channelName) {
		super(channelName);
	}
	
	@Override
	public void receiveObject(Object object, Message msg) {
		if (msg.getMeta().containsKey("um.rpc.method")) {
			String methodName = msg.getMeta("um.rpc.method");
			String inType = msg.getMeta("um.s11n.type");
			String outType = msg.getMeta("um.rpc.outType");
			Object out = null;
			try {
				out = callMethod(methodName, (MessageLite)object, inType, outType);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			Message rpcReplMsg = _rpcPub.prepareMessage(outType, (MessageLite) out);
			rpcReplMsg.putMeta("um.rpc.respId", msg.getMeta("um.rpc.reqId"));
			_rpcPub.send(rpcReplMsg);
		}

	}

	public abstract Object callMethod(String name, MessageLite in, String inType, String outType) throws InterruptedException;

}
