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

import org.umundo.core.Message;

public class ServiceDescription {
	protected String _svcName;
	protected String _channelName = "";
	protected Map<String, String> _properties = new HashMap<String, String>();
	protected ServiceManager _svcMgr;

	public ServiceDescription(Message msg) {
		_svcName = msg.getMeta("um.rpc.desc.name");
		_channelName = msg.getMeta("um.rpc.desc.channel");
		for (String key : msg.getMeta().keySet()) {
			String value = msg.getMeta(key);
			if (key.length() > 12 && key.substring(0, 12).compareTo("um.rpc.desc.") == 0) {
				key = key.substring(12, key.length());
				_properties.put(key, value);
			}
		}
	}

	public ServiceDescription() {
	}
	
	public ServiceDescription(Map<String, String> properties) {
		_properties = properties;
	}

	public Message toMessage() {
		Message msg = new Message();
		for (String key : _properties.keySet()) {
			String value = _properties.get(key);
			msg.putMeta("um.rpc.desc." + key, value);
		}
		msg.putMeta("um.rpc.desc.name", _svcName);
		msg.putMeta("um.rpc.desc.channel", _channelName);
		return msg;
	}

	public String getName() {
		return _svcName;
	}

	public String getChannelName() {
		return _channelName;
	}

	public Map<String, String> getProperties() {
		return _properties;
	}

	public String getProperty(String key) {
		return _properties.get(key);
	}

	public void setProperty(String key, String value) {
		_properties.put(key, value);
	}

	public void setChannelName(String channelName) {
		_channelName = channelName;
	}

	public void setServiceName(String name) {
		_svcName = name;
	}

}
