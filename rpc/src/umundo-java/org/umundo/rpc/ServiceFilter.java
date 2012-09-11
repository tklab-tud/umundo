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
import java.util.UUID;
import java.util.Vector;

import org.umundo.core.Message;
import org.umundo.core.Regex;

public class ServiceFilter {

	public class Rule {
		String key;
		String pattern;
		String value;
		int predicate;
	}

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
		_uuid = msg.getMeta("um.rpc.filter.uuid");
		HashMap<String, String> meta = msg.getMeta();
		for (String key : msg.getMeta().keySet()) {
			String value = msg.getMeta(key);

			if (key.length() > 20 && key.substring(0, 20).compareTo("um.rpc.filter.value.") == 0) {
				try {
					String indexStr = key.substring(20, key.indexOf(".", 20));
					key = key.substring(20 + 1 + indexStr.length(), key.length());

					Rule rule = new Rule();
					rule.key = key;
					rule.value = value;
					rule.predicate = Integer.valueOf(msg.getMeta().get("um.rpc.filter.pred." + indexStr + "." + key));
					rule.pattern = msg.getMeta("um.rpc.filter.pattern." + indexStr + "." + key);
					_rules.add(rule);
				} catch (Exception e) {
					System.err.println(e);
				}
			}
		}
	}

	public Message toMessage() {
		Message msg = new Message();
		msg.putMeta("um.rpc.filter.svcName", _svcName);
		msg.putMeta("um.rpc.filter.uuid", _uuid);

		int i = 0;
		for (Rule rule : _rules) {
			msg.putMeta("um.rpc.filter.value." + i + "." + rule.key, rule.value);
			msg.putMeta("um.rpc.filter.pattern." + i + "." + rule.key, rule.pattern);
			msg.putMeta("um.rpc.filter.pred." + i + "." + rule.key, Integer.toString(rule.predicate));

			i++;
		}
		return msg;
	}

	public void addRule(String key, String value, int pred) {
		addRule(key, ".*", value, pred);
	}

	public void addRule(String key, String pattern, String value, int pred) {
		Rule rule = new Rule();
		rule.key = key;
		rule.pattern = pattern;
		rule.value = value;
		rule.predicate = pred;
		_rules.add(rule);
	}

	public boolean matches(ServiceDescription svcDesc) {
		// service name has to be the same
		if (_svcName.compareTo(svcDesc.getName()) != 0)
			return false;

		// check filter
		for (Rule rule : _rules) {
			String key = rule.key;
			String target = rule.value;
			String pattern = rule.pattern;
			int pred = rule.predicate;
			Regex regex = new Regex(pattern);

			boolean numericMode = false;
			double numTarget = 0;
			double numSubstring = 0;

			if (regex.hasError()) {
				System.err.println("Pattern " + pattern + " does not compile as regular expression");
				return false;
			}

			String actual = svcDesc.getProperty(key);
			// the description says nothing about the given key
			if (actual == null)
				return false;
			
			if (!regex.matches(actual))
				return false;

			// if we matched a substring with (regex) notation, use it
			String substring = "";
			if (regex.hasSubMatches()) {
				substring = regex.getSubMatches().elementAt(0);
			} else {
				substring = regex.getMatch();
			}

			try {
				numTarget = Double.parseDouble(target);
				numSubstring = Double.parseDouble(substring);
				numericMode = true;
			} catch (NumberFormatException nfe) {
			}

			int mod = pred & MASK_MOD;
			int op = pred & MASK_OP;

			switch (op) {
			case OP_EQUALS:
				if (numericMode) {
					if (!(numSubstring == numTarget) ^ ((mod & MOD_NOT) > 0))
						return false;
				} else {
					if (!(substring.compareTo(target) == 0) ^ ((mod & MOD_NOT) > 0))
						return false;
				}
				break;
			case OP_LESS:
				if (numericMode) {
					if (!(numSubstring < numTarget) ^ ((mod & MOD_NOT) > 0))
						return false;
				} else {
					if (!(substring.compareTo(target) < 0) ^ ((mod & MOD_NOT) > 0))
						return false;
				}
				break;
			case OP_GREATER:
				if (numericMode) {
					if (!(numSubstring > numTarget) ^ ((mod & MOD_NOT) > 0))
						return false;
				} else {
					if (!(substring.compareTo(target) > 0) ^ ((mod & MOD_NOT) > 0))
						return false;
				}
				break;
			case OP_STARTS_WITH:
				if (!(substring.startsWith(target)) ^ ((mod & MOD_NOT) > 0))
					return false;
				break;
			case OP_ENDS_WITH:
				if (!(substring.endsWith(target)) ^ ((mod & MOD_NOT) > 0))
					return false;
				break;
			case OP_CONTAINS:
				if (!(substring.contains(target)) ^ ((mod & MOD_NOT) > 0))
					return false;
				break;
			default:
				break;
			}
		}
		return true;
	}

	public String getUUID() {
		return _uuid;
	}

	String _svcName;
	String _uuid;
	Vector<Rule> _rules = new Vector<Rule>();

}
