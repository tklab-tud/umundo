/**
 *  @file
 *  @brief      Facade for finding Nodes in the network.
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

#include "MDNSDiscovery.h"
#include "umundo/core/Factory.h"

namespace umundo {

SharedPtr<Implementation> MDNSDiscovery::create() {
	SharedPtr<Implementation> impl = SharedPtr<Implementation>(new MDNSDiscovery());
	return impl;
}

void MDNSDiscovery::init(const Options* config) {
	// defaults
	_serviceType = "umundo";
	_domain = "local.";
	_protocol = "tcp";

	// override
	if (config != NULL) {
		std::map<std::string, std::string> options = config->getKVPs();
		if (options["mdns.domain"].length() > 0)
			_domain = options["mdns.domain"];
		if (options["mdns.protocol"].length() > 0)
			_protocol = options["mdns.protocol"];
		if (options["mdns.serviceType"].length() > 0)
			_serviceType = options["mdns.serviceType"];
	}

	_mdnsImpl = StaticPtrCast<MDNSDiscoveryImpl>(Factory::create("discovery.mdns.impl"));
	_query = new MDNSQuery();
	_query->domain = _domain;
	_query->regType = "_" + _serviceType + "._" + _protocol + ".";
	_query->rs = this;
	_mdnsImpl->browse(_query);
}

MDNSDiscovery::MDNSDiscovery() : _query(NULL) {
}

MDNSDiscovery::~MDNSDiscovery() {
	// remove all our advertisements
	for (std::map<EndPoint, MDNSAdvertisement*>::iterator adIter = _localAds.begin();
	        adIter != _localAds.end();
	        adIter++) {
		_mdnsImpl->unadvertise(adIter->second);
	}
	if (_mdnsImpl && _query) {
		_mdnsImpl->unbrowse(_query);
	}

	// unreport all found nodes from all queries
	for (std::map<MDNSAdvertisement*, EndPoint>::iterator adIter = _remoteAds.begin();
	        adIter != _remoteAds.end();
	        adIter++) {
		UM_LOG_INFO("MDNS is destroyed, removing %s in %s - notifying nodes", adIter->second.getAddress().c_str(), _domain.c_str());

		for (std::set<ResultSet<ENDPOINT_RS_TYPE>*>::iterator queryIter = _queries.begin();
		        queryIter != _queries.end();
		        queryIter++) {
			(*queryIter)->remove(adIter->second, toStr(this));
		}
	}

	for (std::map<EndPoint, MDNSAdvertisement*>::iterator adIter = _localAds.begin();
	        adIter != _localAds.end();
	        adIter++) {
		delete(adIter->second);
	}

	delete _query;
}

void MDNSDiscovery::suspend() {
}

void MDNSDiscovery::resume() {
}

void MDNSDiscovery::advertise(const EndPoint& node) {

	MDNSAdvertisement* mdnsAd;
	{
		RScopeLock lock(_mutex);
		if (_localAds.find(node) != _localAds.end()) {
			UM_LOG_WARN("Already advertising endpoint");
			return; // we already advertise this one
		}

		mdnsAd = new MDNSAdvertisement();
		mdnsAd->port = node.getPort();
		mdnsAd->regType = "_" + _serviceType + "._" + node.getTransport() + ".";
		mdnsAd->domain = _domain;
		mdnsAd->name = toStr(node.getImpl().get()); // abuse implementation address as name
		_localAds[node] = mdnsAd;
	}
	_mdnsImpl->advertise(mdnsAd);
}

void MDNSDiscovery::add(Node& node) {

	MDNSAdvertisement* mdnsAd;
	{
		RScopeLock lock(_mutex);
		if (_localAds.find(node) != _localAds.end()) {
			UM_LOG_WARN("Node already %s://%s:%d advertised",
			            node.getTransport().c_str(),
			            node.getIP().c_str(),
			            node.getPort());
			return;
		}

		mdnsAd = new MDNSAdvertisement();
		mdnsAd->port = node.getPort();
		mdnsAd->regType = "_" + _serviceType + "._" + node.getTransport() + ".";
		mdnsAd->domain = _domain;
		mdnsAd->name = node.getUUID();
		mdnsAd->isInProcess = true; // this allows for local nodes to use inproc sockets, not actually advertised

		_localAds[node] = mdnsAd;
	}
	_mdnsImpl->advertise(mdnsAd);
	browse(node.getImpl().get());
}

void MDNSDiscovery::unadvertise(const EndPoint& node) {
	RScopeLock lock(_mutex);

	MDNSAdvertisement* mdnsAd;
	{
		RScopeLock lock(_mutex);
		if (_localAds.find(node) == _localAds.end()) {
			UM_LOG_WARN("Not unadvertising %s://%s:%d - node unknown",
			            node.getTransport().c_str(),
			            node.getIP().c_str(),
			            node.getPort());
			return;
		}
		mdnsAd = _localAds[node];
		_localAds.erase(node);
	}
	_mdnsImpl->unadvertise(mdnsAd);
	delete(mdnsAd);
}

void MDNSDiscovery::remove(Node& node) {
	RScopeLock lock(_mutex);

	unbrowse(node.getImpl().get());
	unadvertise(node);
}

void MDNSDiscovery::browse(ResultSet<ENDPOINT_RS_TYPE>* query) {
	RScopeLock lock(_mutex);

	if (_queries.find(query) != _queries.end()) {
		UM_LOG_WARN("Query %p already added for browsing - ignored", query);
		return;
	}
	UM_LOG_INFO("Adding %p query", query);
	_queries.insert(query);

	// report all existing remote endpoints
	for (std::map<MDNSAdvertisement*, EndPoint>::iterator adIter = _remoteAds.begin();
	        adIter != _remoteAds.end();
	        adIter++) {
		query->add(adIter->second, toStr(this));
	}
}

void MDNSDiscovery::unbrowse(ResultSet<ENDPOINT_RS_TYPE>* query) {
	RScopeLock lock(_mutex);

	if (_queries.find(query) == _queries.end()) {
		UM_LOG_WARN("No such query %p to unbrowse - ignored", query);
		return;
	}

	UM_LOG_INFO("Removing %p query", query);
	// unreport all existing remote endpoints
	for (std::map<MDNSAdvertisement*, EndPoint>::iterator adIter = _remoteAds.begin();
	        adIter != _remoteAds.end();
	        adIter++) {
		query->remove(adIter->second, toStr(this));
	}

	_queries.erase(query);
}

/**
 * Called by the actual MDNS implementation when a node is added.
 */
void MDNSDiscovery::added(MDNSAdvertisement* remoteAd) {
	RScopeLock lock(_mutex);

	if (remoteAd->ipv4.size() == 0) {
		UM_LOG_WARN("MDNS reported new node without ipv4 address - ignoring until we found ipv4");
		return;
	}

	if(_remoteAds.find(remoteAd) != _remoteAds.end()) {
		UM_LOG_WARN("MDNS reported existing node %s in %s as new - notifying nodes", _remoteAds[remoteAd].getAddress().c_str(), _domain.c_str());

		// we have already seen this one, report as changed
		for (std::set<ResultSet<ENDPOINT_RS_TYPE>*>::iterator queryIter = _queries.begin();
		        queryIter != _queries.end();
		        queryIter++) {
			(*queryIter)->change(_remoteAds[remoteAd], toStr(this));
		}
	} else {
		// a new one
		EndPoint endPoint(SharedPtr<EndPointImpl>(new EndPointImpl()));

		endPoint.getImpl()->setDomain(remoteAd->domain);
		endPoint.getImpl()->setHost(remoteAd->host);
		endPoint.getImpl()->setInProcess(remoteAd->isInProcess); // figure out

		// not sure what to do if we have multiple ips
		endPoint.getImpl()->setIP(remoteAd->ipv4.begin()->second);

		endPoint.getImpl()->setLastSeen(Thread::getTimeStampMs());
		endPoint.getImpl()->setPort(remoteAd->port);
		endPoint.getImpl()->setRemote(remoteAd->isRemote); // figure out
		endPoint.getImpl()->setUUID(remoteAd->name);

		endPoint.getImpl()->setTransport(remoteAd->getTransport());

		UM_LOG_INFO("MDNS reported new node %s in %s - notifying nodes", endPoint.getAddress().c_str(), _domain.c_str());

		_remoteAds[remoteAd] = endPoint;

		for (std::set<ResultSet<ENDPOINT_RS_TYPE>*>::iterator queryIter = _queries.begin();
		        queryIter != _queries.end();
		        queryIter++) {
			(*queryIter)->add(endPoint, toStr(this));
		}
	}
}

void MDNSDiscovery::removed(MDNSAdvertisement* remoteAd) {
	RScopeLock lock(_mutex);

	if(_remoteAds.find(remoteAd) == _remoteAds.end()) {
		UM_LOG_WARN("MDNS reported vanishing of unknown node %s in %s, might be one missing ipv4 earlier - ignored", remoteAd->host.c_str(), _domain.c_str());
		return;
	}

	UM_LOG_INFO("MDNS reported vanishing of node %s in %s - notifying nodes", _remoteAds[remoteAd].getAddress().c_str(), _domain.c_str());

	for (std::set<ResultSet<ENDPOINT_RS_TYPE>*>::iterator queryIter = _queries.begin();
	        queryIter != _queries.end();
	        queryIter++) {
		(*queryIter)->remove(_remoteAds[remoteAd], toStr(this));
	}
	_remoteAds.erase(remoteAd);
}

void MDNSDiscovery::changed(MDNSAdvertisement* remoteAd, uint64_t what) {
	RScopeLock lock(_mutex);

	if(_remoteAds.find(remoteAd) == _remoteAds.end()) {
		UM_LOG_WARN("MDNS reported unknown node %s in %s as changed - adding as new", _remoteAds[remoteAd].getAddress().c_str(), _domain.c_str());
		added(remoteAd);
		return;
	}

	std::string whatString;
	std::string seperator;
	if (what & Discovery::IFACE_REMOVED) {
		whatString = seperator +  "existing interface gone";
		seperator = ", ";
	}
	if (what & Discovery::IFACE_ADDED) {
		whatString = seperator +  "available on new interface";
		seperator = ", ";
	}

	UM_LOG_INFO("MDNS reported changed node %s in %s as '%s' - notifying nodes", _remoteAds[remoteAd].getAddress().c_str(), _domain.c_str(), whatString.c_str());
	for (std::set<ResultSet<ENDPOINT_RS_TYPE>*>::iterator queryIter = _queries.begin();
	        queryIter != _queries.end();
	        queryIter++) {
		(*queryIter)->change(_remoteAds[remoteAd], toStr(this), what);
	}
}

std::vector<EndPoint> MDNSDiscovery::list() {
	RScopeLock lock(_mutex);

	std::vector<EndPoint> endpoints;
	for (std::map<MDNSAdvertisement*, EndPoint>::iterator adIter = _remoteAds.begin();
	        adIter != _remoteAds.end();
	        adIter++) {
		endpoints.push_back(adIter->second);
	}

	return endpoints;
}

}