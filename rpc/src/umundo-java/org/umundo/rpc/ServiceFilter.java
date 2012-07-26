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
	}

	public ServiceFilter(Message msg) {
		_svcName = msg.getMeta("serviceName");
		for (String key : msg.getMeta().keySet()) {
			if (key.length() > 5 && key.substring(0, 5).compareTo("cond:") == 0) {
				String value = msg.getMeta(key);
				key = key.substring(5, key.length());
				_condition.put(key, value);
				if (msg.getMeta().containsKey("pred:" + key)) {
					_predicate.put(key, Integer.valueOf(msg.getMeta().get("pred:" + key)));
				}
			}
		}
	}

	public Message toMessage() {
		Message msg = new Message();
		msg.putMeta("serviceName", _svcName);
		for (String key : _condition.keySet()) {
			msg.putMeta("cond:" + key, _condition.get(key));
		}
		for (String key : _predicate.keySet()) {
			msg.putMeta("pred:" + key, Integer.toString(_predicate.get(key)));
		}
		return msg;
	}

	public void addRule(String key, String pattern, int pred) {
		_condition.put(key, pattern);
		_predicate.put(key, pred);
	}

	public boolean matches(ServiceDescription svcDesc) {
		// service name has to be the same
		if (_svcName.compareTo(svcDesc.getName()) != 0)
			return false;

		// check filter
		for (String key : _condition.keySet()) {
			String pattern = _condition.get(key);
			String value = svcDesc.getProperty(key);
			int pred = OP_EQUALS;
			if (_predicate.containsKey(key))
				pred = _predicate.get(key);

			switch (pred) {
			case OP_EQUALS:
				if (pattern.compareTo(value) != 0) {
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
	Map<String, String> _condition = new HashMap<String, String>();
	Map<String, Integer> _predicate = new HashMap<String, Integer>();

}
