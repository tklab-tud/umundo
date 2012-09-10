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
import org.umundo.core.Message;

public class ServiceFilter {
	public static final int OP_EQUALS = 0x0001;
	public static final int OP_GREATER = 0x0002;
	public static final int OP_LESS = 0x0003;
	public static final int OP_STARTS_WITH = 0x0004;
	public static final int OP_ENDS_WITH = 0x0005;
	public static final int OP_CONTAINS = 0x0006;
	public static final int MOD_NOT = 0x1000;
	public static final int MASK_OP = 0x0fff;
	public static final int MASK_MOD = 0xf000;

	public ServiceFilter(String svcName) {
		_svcName = svcName;
		_uuid = UUID.randomUUID().toString();
	}

	public ServiceFilter(Message msg) {
		_svcName = msg.getMeta("um.rpc.filter.svcName");
		for (String key : msg.getMeta().keySet()) {
			if (key.length() > 20 && key.substring(0, 20).compareTo("um.rpc.filter.value.") == 0) {
				String value = msg.getMeta(key);
				key = key.substring(20, key.length());
				_value.put(key, value);
				_pattern.put(key, msg.getMeta("um.rpc.filter.pattern." + key));
				_predicate.put(key, Integer.valueOf(msg.getMeta("um.rpc.filter.pred." + key)));
			}
		}
	}

	public Message toMessage() {
		Message msg = new Message();
		msg.putMeta("um.rpc.filter.svcName", _svcName);
		msg.putMeta("um.rpc.filter.uuid", _uuid);
		for (String key : _value.keySet()) {
			msg.putMeta("um.rpc.filter.value." + key, _value.get(key));
		}

		for (String key : _pattern.keySet()) {
			msg.putMeta("um.rpc.filter.pattern." + key, _pattern.get(key));
		}
		
		for (String key : _predicate.keySet()) {
			msg.putMeta("um.rpc.filter.pred." + key, _predicate.get(key).toString());
		}
		return msg;
	}

	public void addRule(String key, String value, int pred) {
		addRule(key, ".*", value, pred);
	}

	public void addRule(String key, String pattern, String value, int pred) {
		_pattern.put(key, pattern);
		_value.put(key, value);
		_predicate.put(key, pred);
	}

	public boolean matches(ServiceDescription svcDesc) {
		// service name has to be the same
		if (_svcName.compareTo(svcDesc.getName()) != 0)
			return false;

		// check filter
		for (String key : _value.keySet()) {
			String actual = svcDesc.getProperty(key);  // the actual string as it is present in the description
			String target = _value.get(key);                // the substring from the filter
			String pattern = _pattern.get(key);             // the pattern that will transform the actual string into a substring
			int pred = _predicate.get(key);                 // the relation between filter and description sting

			if (_predicate.containsKey(key))
				pred = _predicate.get(key);

			switch (pred) {
			case OP_EQUALS:
				if (pattern.compareTo(target) != 0) {
					return false;
				}
				break;

			default:
				break;
			}
		}
		return true;
	}

	String _svcName;
	String _uuid;
	Map<String, String> _pattern = new HashMap<String, String>();
	Map<String, String> _value = new HashMap<String, String>();
	Map<String, Integer> _predicate = new HashMap<String, Integer>();

}
