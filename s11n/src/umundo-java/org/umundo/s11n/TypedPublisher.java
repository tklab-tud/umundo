/**
 *  Copyright (C) 2012  Daniel Schreiber
 *  Copyright (C) 2012  Stefan Radomski
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

package org.umundo.s11n;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

import org.umundo.core.Message;
import org.umundo.core.Publisher;
import org.umundo.core.Greeter;
import org.umundo.s11n.ITypedGreeter;

import com.google.protobuf.MessageLite;

public class TypedPublisher extends Publisher {

	ITypedGreeter typedGreeter;
	GreeterDecorator greeterDecorator;

	class GreeterDecorator extends Greeter {
		public void welcome(Publisher pub, String nodeId, String subId) {
			if (TypedPublisher.this.typedGreeter != null) {
				TypedPublisher.this.typedGreeter.welcome(TypedPublisher.this, nodeId, subId);
			}
		}

		public void farewell(Publisher pub, String nodeId, String subId) {
			if (TypedPublisher.this.typedGreeter != null) {
				TypedPublisher.this.typedGreeter.farewell(TypedPublisher.this, nodeId, subId);
			}
		}	
	}

	public TypedPublisher(String channel) {
		super(channel);
	}
	
	public Message prepareMessage(MessageLite o) {
			return prepareMessage(o.getClass().getSimpleName(), o);
	}
	
  public Message prepareMessage(String type, MessageLite o) {
		Message msg = new Message();
		byte[] buffer = o.toByteArray();
		msg.setData(buffer);
		msg.putMeta("um.s11n.type", type);		
		return msg;
	}
	
	public void sendObject(MessageLite o) {
		sendObject(o.getClass().getName(), o);
	}
	
	public void sendObject(String type, MessageLite o) {
	  send(prepareMessage(type, o));
	}

	public void setGreeter(Greeter greeter) {
		System.err.println("Ignoring call to setGreeter(Greeter): use a TypedGreeter with a TypedPublisher");
	}
	
	public void setGreeter(ITypedGreeter greeter) {
		if (greeterDecorator == null) {
			greeterDecorator = new GreeterDecorator();
			super.setGreeter(greeterDecorator);
		}

		typedGreeter = greeter;
	}
	
}
