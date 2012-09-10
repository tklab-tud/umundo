/**
 *  @file
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
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
 *  @endcond
 */

#include "umundo/rpc/Service.h"
#include "umundo/rpc/ServiceManager.h"
#include "umundo/common/Regex.h"
#include <stdlib.h>
#include <sstream>
#include <ctype.h>

namespace umundo {

ServiceFilter::ServiceFilter(const string& svcName) {
	_svcName = svcName;
	_uuid = UUID::getUUID();
}

Message* ServiceFilter::toMessage() {
	Message* msg = new Message();
	msg->putMeta("um.rpc.filter.svcName", _svcName);
	msg->putMeta("um.rpc.filter.uuid", _uuid);

	map<string, string>::iterator valIter = _value.begin();
	while(valIter != _value.end()) {
		msg->putMeta("um.rpc.filter.value." + valIter->first, valIter->second);
		valIter++;
	}

	map<string, string>::iterator patternIter = _pattern.begin();
	while(patternIter != _pattern.end()) {
		msg->putMeta("um.rpc.filter.pattern." + patternIter->first, patternIter->second);
		patternIter++;
	}

	map<string, int>::iterator predIter = _predicate.begin();
	while(predIter != _predicate.end()) {
		std::stringstream ss;
		ss << predIter->second;
		msg->putMeta("um.rpc.filter.pred." + predIter->first, ss.str());
		predIter++;
	}
	return msg;
}

ServiceFilter::ServiceFilter(Message* msg) {
	_svcName = msg->getMeta("um.rpc.filter.svcName");
	_uuid = msg->getMeta("um.rpc.filter.uuid");
	map<string, string> meta = msg->getMeta();
	map<string, string>::const_iterator metaIter = meta.begin();
	while(metaIter != meta.end()) {
		string key = metaIter->first;
		string value = metaIter->second;

		if (key.compare(0, 20, "um.rpc.filter.value.") == 0) {
			key = key.substr(20, key.length());
			assert(meta.find("um.rpc.filter.pattern." + key) != meta.end());
			assert(meta.find("um.rpc.filter.pred." + key) != meta.end());

			_value[key] = value;
			_pattern[key] = meta["um.rpc.filter.pattern." + key];
			_predicate[key] = (Predicate)atoi(meta["um.rpc.filter.pred." + key].c_str());
		}
		metaIter++;
	}
}

void ServiceFilter::addRule(const string& key, const string& value, int pred) {
	addRule(key, ".*", value, pred);
}

void ServiceFilter::addRule(const string& key, const string& pattern, const string& value, int pred) {
	_pattern[key] = pattern;
	_value[key] = value;
	_predicate[key] = pred;
}

void ServiceFilter::clearRules() {
	_pattern.clear();
	_value.clear();
	_predicate.clear();
}

bool ServiceFilter::matches(ServiceDescription* svcDesc) {
	// service name has to be the same
	if (_svcName.compare(svcDesc->getName()) != 0)
		return false;

	// check filter
	map<string, string>::iterator condIter = _value.begin();
	while(condIter != _value.end()) {

		/* A condition is true, if the matched substring from the value for key of
		 * the service description is in the relation given by the predicate to the
		 * filter value.
		 */

		string key = condIter->first;               // the key for the values
		string actual = svcDesc->getProperty(key);  // the actual string as it is present in the description
		string target = _value[key];                // the substring from the filter
		string pattern = _pattern[key];             // the pattern that will transform the actual string into a substring
		int pred = _predicate[key];                 // the relation between filter and description sting
		Regex regex(pattern);

		bool numericMode = false;
		double numTarget = 0;
		double numSubstring = 0;

		if (regex.hasError()) {
			LOG_ERR("Pattern '%s' does not compile as regular expression", pattern.c_str());
			return false;
		}

		if (!regex.matches(actual))
			return false;

		// if we matched a substring with (regex) notation, use it
		string substring;
		if (regex.hasSubMatches()) {
			substring = actual.substr(regex.getSubMatches()[0].first, regex.getSubMatches()[0].second);
		} else {
			substring = actual.substr(regex.getMatch().first, regex.getMatch().second);
		}

		// use numeric mode with OP_EQUALS, OP_LESS and OP_GREATER?
		if (isNumeric(target) && isNumeric(substring)) {
			numericMode = true;
			numTarget = toNumber(target);
			numSubstring = toNumber(substring);
		}

		int mod = pred & MASK_MOD;
		int op = pred & MASK_OP;

#if 0
		// we used this to debug the boolean expressions in the switch
		if (substring.find(target) == substring.length() - target.length()) {
			printf("EQUALS: 1\n");
		} else {
			printf("EQUALS: 0\n");
		}
		if (mod & MOD_NOT) {
			printf("NOT: 1\n");
		} else {
			printf("NOT: 0\n");
		}

		if (true ^ true)
			printf("true, true\n");
		if (false ^ true)
			printf("false, true\n");
		if (true ^ false)
			printf("true, false\n");
		if (false ^ false)
			printf("false, false\n");
#endif

		switch (op) {
		case OP_EQUALS:
			if (numericMode) {
				if (!(numSubstring == numTarget) ^ ((mod & MOD_NOT) > 0))
					return false;
			} else {
				if (!(substring.compare(target) == 0) ^ ((mod & MOD_NOT) > 0))
					return false;
			}
			break;
		case OP_LESS:
			if (numericMode) {
				if (!(numSubstring < numTarget) ^ ((mod & MOD_NOT) > 0))
					return false;
			} else {
				if (!(substring.compare(target) < 0) ^ ((mod & MOD_NOT) > 0))
					return false;
			}
			break;
		case OP_GREATER:
			if (numericMode) {
				if (!(numSubstring > numTarget) ^ ((mod & MOD_NOT) > 0))
					return false;
			} else {
				if (!(substring.compare(target) > 0) ^ ((mod & MOD_NOT) > 0))
					return false;
			}
			break;
		case OP_STARTS_WITH:
			if (!(substring.find(target) == 0) ^ ((mod & MOD_NOT) > 0))
				return false;
			break;
		case OP_ENDS_WITH:
			if (!(substring.find(target) == substring.length() - target.length()) ^ ((mod & MOD_NOT) > 0))
				return false;
			break;
		case OP_CONTAINS:
			if (!(substring.find(target) != std::string::npos) ^ ((mod & MOD_NOT) > 0))
				return false;
			break;
		default:
			break;
		}

		condIter++;
	}
	return true;
}

bool ServiceFilter::isNumeric(const string& test) {
	string::const_iterator sIter = test.begin();
	for(; sIter != test.end(); sIter++) {
		if (isdigit(*sIter))
			continue;
		if ((*sIter) == '.')
			continue;
		if ((*sIter) == 'e')
			continue;
		return false;
	}
	return true;
}

double ServiceFilter::toNumber(const string& numberString) {
	std::istringstream os(numberString);
	double d;
	os >> d;
	return d;
}

ServiceDescription::ServiceDescription(const string& svcName) {
	_svcName = svcName;
}

ServiceDescription::ServiceDescription(const string& svcName, map<string, string> properties) {
	_svcName = svcName;
	_properties = properties;
}

ServiceDescription::ServiceDescription(Message* msg) {
	_svcName = msg->getMeta("um.rpc.desc.name");
	_channelName = msg->getMeta("um.rpc.desc.channel");
	map<string, string>::const_iterator metaIter = msg->getMeta().begin();
	while(metaIter != msg->getMeta().end()) {
		string key = metaIter->first;
		string value = metaIter->second;
		if (key.length() > 12 && key.compare(0, 12, "um.rpc.desc.") == 0) {
			key = key.substr(12, key.length());
			_properties[key] = value;
		}
		metaIter++;
	}
}

Message* ServiceDescription::toMessage() {
	Message* msg = new Message();
	map<string, string>::const_iterator propIter = _properties.begin();
	while(propIter != _properties.end()) {
		msg->putMeta("um.rpc.desc." + propIter->first, propIter->second);
		propIter++;
	}
	msg->putMeta("um.rpc.desc.name", _svcName);
	msg->putMeta("um.rpc.desc.channel", _channelName);
	return msg;
}

ServiceStub::ServiceStub(ServiceDescription* svcDesc) {
	_channelName = svcDesc->getChannelName();
	_rpcPub = new TypedPublisher(_channelName);
	_rpcSub = new TypedSubscriber(_channelName, this);

	set<Node*> nodes = svcDesc->getServiceManager()->getNodes();
	set<Node*>::iterator nodeIter = nodes.begin();
	while(nodeIter != nodes.end()) {
		(*nodeIter)->connect(this);
		nodeIter++;
	}
	_rpcPub->waitForSubscribers(1);
}

ServiceStub::ServiceStub(const string& channelName) {
	_channelName = channelName;
	_rpcPub = new TypedPublisher(_channelName);
	_rpcSub = new TypedSubscriber(_channelName, this);
	_rpcPub->waitForSubscribers(1);

}

ServiceStub::~ServiceStub() {
	delete _rpcPub;
	delete _rpcSub;
}

std::set<umundo::Publisher*> ServiceStub::getPublishers() {
	set<Publisher*> pubs;
	pubs.insert(_rpcPub);
	return pubs;
}
std::set<umundo::Subscriber*> ServiceStub::getSubscribers() {
	set<Subscriber*> subs;
	subs.insert(_rpcSub);
	return subs;
}

const string& ServiceStub::getChannelName() {
	return _channelName;
}

const string& ServiceStub::getName() {
	return _serviceName;
}

void ServiceStub::callStubMethod(const string& name, void* in, const string& inType, void* &out, const string& outType) {
	Message* rpcReqMsg = _rpcPub->prepareMsg(inType, in);
	string reqId = UUID::getUUID();
	rpcReqMsg->putMeta("um.rpc.reqId", reqId);
	rpcReqMsg->putMeta("um.rpc.method", name);
	rpcReqMsg->putMeta("um.rpc.outType", outType);
	assert(_requests.find(reqId) == _requests.end());
	_requests[reqId] = new Monitor();
	_rpcPub->send(rpcReqMsg);
	ScopeLock lock(_mutex);
	_requests[reqId]->wait(_mutex);
	delete _requests[reqId];
	_requests.erase(reqId);
	out = _responses[reqId];
	_responses.erase(reqId);
	delete rpcReqMsg;
}


void ServiceStub::receive(void* obj, Message* msg) {
	if (msg->getMeta().find("um.rpc.respId") != msg->getMeta().end()) {
		string respId = msg->getMeta("um.rpc.respId");
		ScopeLock lock(_mutex);
		if (_requests.find(respId) != _requests.end()) {
			_responses[respId] = obj;
			_requests[respId]->signal();
		}
	}
}


Service::Service() {
	_channelName = UUID::getUUID();
	_rpcPub = new TypedPublisher(_channelName);
	_rpcSub = new TypedSubscriber(_channelName, this);
}

Service::~Service() {

}

void Service::receive(void* obj, Message* msg) {
	// somone wants a method called
	if (msg->getMeta().find("um.rpc.method") != msg->getMeta().end()) {
		string methodName = msg->getMeta("um.rpc.method");
		string inType = msg->getMeta("um.s11n.type");
		string outType = msg->getMeta("um.rpc.outType");
		void* out = NULL;
		callMethod(methodName, obj, inType, out, outType);
		if (out != NULL) {
			Message* rpcReplMsg = _rpcPub->prepareMsg(outType, out);
			rpcReplMsg->putMeta("um.rpc.respId", msg->getMeta("um.rpc.reqId"));
			_rpcPub->send(rpcReplMsg);
			delete rpcReplMsg;
		}
		cleanUpObjects(methodName, obj, out);
	}
}

}