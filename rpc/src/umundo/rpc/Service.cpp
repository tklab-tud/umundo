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

ServiceFilter::ServiceFilter(const std::string& svcName) {
	_svcName = svcName;
	_uuid = UUID::getUUID();
}

Message* ServiceFilter::toMessage() const  {
	Message* msg = new Message();
	msg->putMeta("um.rpc.filter.svcName", _svcName);
	msg->putMeta("um.rpc.filter.uuid", _uuid);

	std::vector<Rule>::const_iterator ruleIter = _rules.begin();
	int i = 0;
	while(ruleIter != _rules.end()) {
		std::stringstream ssValueKey;
		std::stringstream ssPatternKey;
		std::stringstream ssPredKey;

		ssValueKey << "um.rpc.filter.value." << i << "." << ruleIter->key;
		msg->putMeta(ssValueKey.str(), ruleIter->value);

		ssPatternKey << "um.rpc.filter.pattern." << i << "." << ruleIter->key;
		msg->putMeta(ssPatternKey.str(), ruleIter->pattern);

		std::stringstream ssPredValue;
		ssPredValue << ruleIter->predicate;

		ssPredKey << "um.rpc.filter.pred." << i << "." << ruleIter->key;
		msg->putMeta(ssPredKey.str(), ssPredValue.str());

		i++;
		ruleIter++;
	}
	return msg;
}

ServiceFilter::ServiceFilter(Message* msg) {
	_svcName = msg->getMeta("um.rpc.filter.svcName");
	_uuid = msg->getMeta("um.rpc.filter.uuid");
	std::map<std::string, std::string> meta = msg->getMeta();
	std::map<std::string, std::string>::const_iterator metaIter = meta.begin();
	while(metaIter != meta.end()) {
		std::string key = metaIter->first;
		std::string value = metaIter->second;

		if (key.size() > 20 && key.compare(0, 20, "um.rpc.filter.value.") == 0) {
			size_t dotPos = key.find(".", 20);
			if (dotPos == std::string::npos)
				continue;

			std::string indexStr(key.substr(20, dotPos - 20));

			key = key.substr(20 + 1 + indexStr.length(), key.length() - (20 + indexStr.length()));
			assert(meta.find("um.rpc.filter.pattern." + indexStr + "." + key) != meta.end());
			assert(meta.find("um.rpc.filter.pred." + indexStr + "." + key) != meta.end());

			Rule rule;
			rule.key = key;
			rule.value = value;
			rule.predicate = (Predicate)atoi(meta["um.rpc.filter.pred." + indexStr + "." + key].c_str());
			rule.pattern = meta["um.rpc.filter.pattern." + indexStr + "." + key];

			_rules.push_back(rule);
		}
		metaIter++;
	}
}

void ServiceFilter::addRule(const std::string& key, const std::string& value, int pred) {
	addRule(key, ".*", value, pred);
}

void ServiceFilter::addRule(const std::string& key, const std::string& pattern, const std::string& value, int pred) {
	// @TODO: we cannot use a map here!
	Rule rule;
	rule.pattern = pattern;
	rule.key = key;
	rule.value = value;
	rule.predicate = pred;
	_rules.push_back(rule);
}

void ServiceFilter::clearRules() {
	_rules.clear();
}

bool ServiceFilter::matches(const ServiceDescription& svcDesc) const {
	// service name has to be the same
	if (_svcName.compare(svcDesc.getName()) != 0)
		return false;

	// check filter
	std::vector<Rule>::const_iterator ruleIter = _rules.begin();
	while(ruleIter != _rules.end()) {

		/* A condition is true, if the matched substring from the value for key of
		 * the service description is in the relation given by the predicate to the
		 * filter value.
		 */

		std::string key = ruleIter->key;                     // the key for the values
		if (!svcDesc.hasProperty(key)) {
			ruleIter++;
			continue;
		}

		std::string actual = svcDesc.getProperty(key);      // the actual string as it is present in the description
		std::string target = ruleIter->value;                // the substring from the filter
		std::string pattern = ruleIter->pattern;             // the pattern that will transform the actual string into a substring
		int pred = ruleIter->predicate;                 // the relation between filter and description sting
		Regex regex(pattern);

		bool numericMode = false;
		double numTarget = 0;
		double numSubstring = 0;

		if (regex.hasError()) {
			UM_LOG_ERR("Pattern '%s' does not compile as regular expression", pattern.c_str());
			return false;
		}

		if (!regex.matches(actual))
			return false;

		// if we matched a substring with (regex) notation, use it
		std::string substring;
		if (regex.hasSubMatches()) {
			substring = regex.getSubMatches()[0];
		} else {
			substring = regex.getMatch();
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

		ruleIter++;
	}
	return true;
}

bool ServiceFilter::isNumeric(const std::string& test) const {
	std::string::const_iterator sIter = test.begin();
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

double ServiceFilter::toNumber(const std::string& numberString) const {
	std::istringstream os(numberString);
	double d;
	os >> d;
	return d;
}

ServiceDescription::ServiceDescription() {
}

ServiceDescription::ServiceDescription(std::map<std::string, std::string> properties) {
	_properties = properties;
}

ServiceDescription::ServiceDescription(Message* msg) {
	_svcName = msg->getMeta("um.rpc.desc.name");
	_channelName = msg->getMeta("um.rpc.desc.channel");
	assert(_svcName.size() > 0);
	assert(_channelName.size() > 0);
	std::map<std::string, std::string>::const_iterator metaIter = msg->getMeta().begin();
	while(metaIter != msg->getMeta().end()) {
		std::string key = metaIter->first;
		std::string value = metaIter->second;
		if (key.length() > 12 && key.compare(0, 12, "um.rpc.desc.") == 0) {
			key = key.substr(12, key.length());
			_properties[key] = value;
		}
		metaIter++;
	}
}

Message* ServiceDescription::toMessage() const {
	Message* msg = new Message();
	std::map<std::string, std::string>::const_iterator propIter = _properties.begin();
	while(propIter != _properties.end()) {
		msg->putMeta("um.rpc.desc." + propIter->first, propIter->second);
		propIter++;
	}
	assert(_svcName.size() > 0);
	assert(_channelName.size() > 0);
	msg->putMeta("um.rpc.desc.name", _svcName);
	msg->putMeta("um.rpc.desc.channel", _channelName);
	return msg;
}

/**
 * Constructor for local service stubs
 */
ServiceStub::ServiceStub(const ServiceDescription& svcDesc) {
	UM_LOG_INFO("Creating new service stub for '%s' at %s.[listen|serve]", svcDesc.getName().c_str(), svcDesc.getChannelName().c_str());
	_channelName = svcDesc.getChannelName();
	_rpcPub = TypedPublisher(_channelName + ".listen");
	_rpcSub = TypedSubscriber(_channelName + ".serve", this);

	std::set<Node> nodes = svcDesc.getServiceManager()->getNodes();
	std::set<Node>::iterator nodeIter = nodes.begin();
	while(nodeIter != nodes.end()) {
		((Node)*nodeIter).connect(this);
		nodeIter++;
	}
	_rpcPub.waitForSubscribers(1);
}

ServiceStub::ServiceStub(const std::string& channelName) {
	_channelName = channelName;
	_rpcPub = TypedPublisher(_channelName + ".listen");
	_rpcSub = TypedSubscriber(_channelName + ".serve", this);
	_rpcPub.waitForSubscribers(1);
}

ServiceStub::~ServiceStub() {
}

std::map<std::string, Publisher> ServiceStub::getPublishers() {
	std::map<std::string, Publisher> pubs;
	pubs[_rpcPub.getUUID()] = _rpcPub;
	return pubs;
}
std::map<std::string, Subscriber> ServiceStub::getSubscribers() {
	std::map<std::string, Subscriber> subs;
	subs[_rpcSub.getUUID()] = _rpcSub;
	return subs;
}

const std::string ServiceStub::getChannelName() {
	return _channelName;
}

const std::string ServiceStub::getName() {
	return _serviceName;
}

void ServiceStub::callStubMethod(const std::string& name, void* in, const std::string& inType, void* &out, const std::string& outType) {
	Message* rpcReqMsg = _rpcPub.prepareMsg(inType, in);
	std::string reqId = UUID::getUUID();
	rpcReqMsg->putMeta("um.rpc.reqId", reqId);
	rpcReqMsg->putMeta("um.rpc.method", name);
	rpcReqMsg->putMeta("um.rpc.outType", outType);
	assert(_requests.find(reqId) == _requests.end());
	_requests[reqId] = new Monitor();
	_rpcPub.send(rpcReqMsg);
	RScopeLock lock(_mutex);

	int retries = 5;
	while(retries-- > 0) {
		_requests[reqId]->wait(_mutex, 1000);
		if (_responses.find(reqId) != _responses.end()) {
			break;
		}
		UM_LOG_ERR("Calling %s did not return within 1s - retrying", name.c_str());
	}

	if (_requests.find(reqId) == _requests.end()) {
		UM_LOG_ERR("Did not get a reply for calling %s", name.c_str());
	}

	delete _requests[reqId];
	_requests.erase(reqId);
	out = _responses[reqId];
	_responses.erase(reqId);
	delete rpcReqMsg;
}


void ServiceStub::receive(void* obj, Message* msg) {
	if (msg->getMeta().find("um.rpc.respId") != msg->getMeta().end()) {
		std::string respId = msg->getMeta("um.rpc.respId");
		RScopeLock lock(_mutex);
		if (_requests.find(respId) != _requests.end()) {
			_responses[respId] = obj;
			_requests[respId]->signal();
		}
	}
}


/**
 * This constructor is called for local service instantiations
 */
Service::Service() {
	_channelName = UUID::getUUID();
	_rpcPub = TypedPublisher(_channelName + ".serve");
	_rpcSub = TypedSubscriber(_channelName + ".listen", this);
	UM_LOG_INFO("Creating new service at %s.[listen|serve]", _channelName.c_str());
}

Service::~Service() {

}

void Service::receive(void* obj, Message* msg) {
	// somone wants a method called
	if (msg->getMeta().find("um.rpc.method") != msg->getMeta().end()) {
		std::string methodName = msg->getMeta("um.rpc.method");
		std::string inType = msg->getMeta("um.s11n.type");
		std::string outType = msg->getMeta("um.rpc.outType");
		void* out = NULL;
		callMethod(methodName, obj, inType, out, outType);
		if (out != NULL) {
			Message* rpcReplMsg = _rpcPub.prepareMsg(outType, out);
			rpcReplMsg->putMeta("um.rpc.respId", msg->getMeta("um.rpc.reqId"));
			_rpcPub.send(rpcReplMsg);
			delete rpcReplMsg;
		}
		cleanUpObjects(methodName, obj, out);
	}
}

}