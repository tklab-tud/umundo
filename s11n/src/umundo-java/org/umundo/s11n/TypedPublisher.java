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

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.umundo.core.Greeter;
import org.umundo.core.Message;
import org.umundo.core.Publisher;

import com.google.protobuf.DescriptorProtos.FileDescriptorProto;
import com.google.protobuf.DescriptorProtos.FileDescriptorSet;
import com.google.protobuf.Descriptors.Descriptor;
import com.google.protobuf.Descriptors.DescriptorValidationException;
import com.google.protobuf.Descriptors.FileDescriptor;
import com.google.protobuf.Descriptors.ServiceDescriptor;
import com.google.protobuf.DynamicMessage.Builder;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.MessageLite;

public class TypedPublisher extends Publisher {

	ITypedGreeter typedGreeter;
	GreeterDecorator greeterDecorator;

	class GreeterDecorator extends Greeter {
		public void welcome(Publisher pub, String nodeId, String subId) {
			if (TypedPublisher.this.typedGreeter != null) {
				TypedPublisher.this.typedGreeter.welcome(TypedPublisher.this,
						nodeId, subId);
			}
		}

		public void farewell(Publisher pub, String nodeId, String subId) {
			if (TypedPublisher.this.typedGreeter != null) {
				TypedPublisher.this.typedGreeter.farewell(TypedPublisher.this,
						nodeId, subId);
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
		System.err
				.println("Ignoring call to setGreeter(Greeter): use a TypedGreeter with a TypedPublisher");
	}

	public void setGreeter(ITypedGreeter greeter) {
		if (greeterDecorator == null) {
			greeterDecorator = new GreeterDecorator();
			super.setGreeter(greeterDecorator);
		}

		typedGreeter = greeter;
	}

	public static Descriptor protoDescForMessage(String typeName) {
		return protoMsgDesc.get(typeName);
	}

	public static ServiceDescriptor protoDescForService(String typeName) {
		return protoSvcDesc.get(typeName);
	}

	public static void addProtoDesc(File dirOrFile) throws IOException,
			DescriptorValidationException {
		if (!dirOrFile.exists()) {
			System.err.println("No such file or directory: "
					+ dirOrFile.getName());
		}
		if (dirOrFile.isDirectory()) {
			for (File file : dirOrFile.listFiles()) {
				addProtoDesc(file);
			}
			return;
		}
		if (dirOrFile.isFile() && dirOrFile.getName().endsWith(".desc")) {
			// open file and parse as file descriptor set
			final FileDescriptorSet fdSet = FileDescriptorSet
					.parseFrom(new FileInputStream(dirOrFile));
			FileDescriptor current = null;
			// iterate all file descriptors from desc file
			for (FileDescriptorProto fdProto : fdSet.getFileList()) {
				final List<String> depList = fdProto.getDependencyList();
				final FileDescriptor[] fdDeps = new FileDescriptor[depList
						.size()];

				// iterate all dependencies and get their file descriptors
				for (int i = 0; i < fdDeps.length; i++) {
					// assume that we already saw the desc file or
					// include_imports was specified
					FileDescriptor fdDep = protoFile.get(depList.get(i));
					if (fdDep == null) {
						throw new InvalidProtocolBufferException(
								fdProto.getName()
										+ ": depends on unknown message "
										+ depList.get(i)
										+ ", add it before or use protoc with --include_imports");
					} else {
						fdDeps[i] = fdDep;
					}
				}
				current = FileDescriptor.buildFrom(fdProto, fdDeps);

				// remember all contained messages
				for (Descriptor desc : current.getMessageTypes()) {
					protoMsgDesc.put(desc.getName(), desc);
				}
				// remember all contained services
				for (ServiceDescriptor desc : current.getServices()) {
					protoSvcDesc.put(desc.getName(), desc);
				}

				protoFile.put(current.getName(), current);
			}
		} else {
			System.err.println("Ignoring " + dirOrFile.getName());
		}
	}

	private static Map<String, FileDescriptor> protoFile = new HashMap<String, FileDescriptor>();
	private static Map<String, Descriptor> protoMsgDesc = new HashMap<String, Descriptor>();
	private static Map<String, Builder> protoMsgBuilders = new HashMap<String, Builder>();
	private static Map<String, ServiceDescriptor> protoSvcDesc = new HashMap<String, ServiceDescriptor>();

}
