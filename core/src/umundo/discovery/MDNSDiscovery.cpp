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
#include "umundo/common/Factory.h"

namespace umundo {
  
boost::shared_ptr<Implementation> MDNSDiscovery::create() {
	boost::shared_ptr<Implementation> impl = boost::shared_ptr<Implementation>(new MDNSDiscovery());
	return impl;
}

void MDNSDiscovery::init(Options* config) {
	if (config != NULL)
		_config = config->getKVPs();

	// make sure these are set
	if (_config["mdns.domain"].length() == 0)       _config["mdns.domain"] = "local.";
	if (_config["mdns.protocol"].length() == 0)     _config["mdns.protocol"] = "tcp";
	if (_config["mdns.serviceType"].length() == 0)  _config["mdns.serviceType"] = "umundo";

	_mdnsImpl = boost::static_pointer_cast<MDNSDiscoveryImpl>(Factory::create("discovery.mdns.impl"));
	_query = new MDNSQuery();
	_query->domain = _config["mdns.domain"];
	_query->regType = "_" + _config["mdns.serviceType"] + "._" + _config["mdns.protocol"] + ".";
	_query->rs = this;
	_mdnsImpl->browse(_query);
}

MDNSDiscovery::MDNSDiscovery() : _query(NULL) {
}

MDNSDiscovery::~MDNSDiscovery() {
	// remove all our advertisements
	for (std::map<EndPoint, MDNSAd*>::iterator adIter = _localAds.begin();
			 adIter != _localAds.end();
			 adIter++) {
		_mdnsImpl->unadvertise(adIter->second);
	}
	if (_mdnsImpl && _query) {
		_mdnsImpl->unbrowse(_query);
	}
	
	// unreport all found nodes from all queries
	for (std::map<MDNSAd*, EndPoint>::iterator adIter = _remoteAds.begin();
			 adIter != _remoteAds.end();
			 adIter++) {
		for (std::set<ResultSet<EndPoint>*>::iterator queryIter = _queries.begin();
				 queryIter != _queries.end();
				 queryIter++) {
			(*queryIter)->removed(adIter->second);
		}
	}

	delete _query;
}
	
void MDNSDiscovery::suspend() {
}

void MDNSDiscovery::resume() {
}

void MDNSDiscovery::advertise(const EndPoint& node) {

	MDNSAd* mdnsAd;
	{
		ScopeLock lock(_mutex);
		if (_localAds.find(node) != _localAds.end()) {
			UM_LOG_WARN("Already advertising endpoint");
			return; // we already advertise this one
		}

		mdnsAd = new MDNSAd();
		mdnsAd->port = node.getPort();
		mdnsAd->regType = "_" + _config["mdns.serviceType"] + "._" + node.getTransport() + ".";
		mdnsAd->domain = _config["mdns.domain"];
		mdnsAd->name = toStr(node.getImpl().get()); // abuse implementation address
		_localAds[node] = mdnsAd;
	}
	_mdnsImpl->advertise(mdnsAd);
}

void MDNSDiscovery::add(Node& node) {

	MDNSAd* mdnsAd;
	{
		ScopeLock lock(_mutex);
		if (_localAds.find(node) != _localAds.end()) {
			UM_LOG_WARN("Node already %s://%s:%d advertised",
									node.getTransport().c_str(),
									node.getIP().c_str(),
									node.getPort());
			return;
		}

		mdnsAd = new MDNSAd();
		mdnsAd->port = node.getPort();
		mdnsAd->regType = "_" + _config["mdns.serviceType"] + "._" + node.getTransport() + ".";
		mdnsAd->domain = _config["mdns.domain"];
		mdnsAd->name = node.getUUID();
		mdnsAd->isInProcess = true; // this allows for local nodes to use inproc sockets, not actually advertised
		
		_localAds[node] = mdnsAd;
	}
	_mdnsImpl->advertise(mdnsAd);
	browse(node.getImpl().get());
}

void MDNSDiscovery::unadvertise(const EndPoint& node) {

	MDNSAd* mdnsAd;
	{
		ScopeLock lock(_mutex);
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
	unbrowse(node.getImpl().get());
	unadvertise(node);
}

void MDNSDiscovery::browse(ResultSet<EndPoint>* query) {
	ScopeLock lock(_mutex);

	if (_queries.find(query) != _queries.end()) {
		UM_LOG_WARN("Query %p already added for browsing", query);
		return;
	}
	UM_LOG_INFO("Adding %p query", query);
	_queries.insert(query);

	// report all existing remote endpoints
	for (std::map<MDNSAd*, EndPoint>::iterator adIter = _remoteAds.begin();
			 adIter != _remoteAds.end();
			 adIter++) {
		query->added(adIter->second);
	}
}

void MDNSDiscovery::unbrowse(ResultSet<EndPoint>* query) {
	ScopeLock lock(_mutex);

	if (_queries.find(query) == _queries.end()) {
		UM_LOG_WARN("No such query %p to unbrowse", query);
		return;
	}
	
	UM_LOG_INFO("Removing %p query", query);
	// unreport all existing remote endpoints
	for (std::map<MDNSAd*, EndPoint>::iterator adIter = _remoteAds.begin();
			 adIter != _remoteAds.end();
			 adIter++) {
		query->removed(adIter->second);
	}

	_queries.erase(query);
}

/**
 * Called by the actual MDNS implementation when a node is added.
 */
void MDNSDiscovery::added(MDNSAd* remoteAd) {
	ScopeLock lock(_mutex);

	if(_remoteAds.find(remoteAd) != _remoteAds.end()) {
		UM_LOG_WARN("MDNS reported existing node %s in %s as new", _remoteAds[remoteAd].getAddress().c_str(), _config["mdns.domain"].c_str());

		// we have already seen this one, report as changed
		for (std::set<ResultSet<EndPoint>*>::iterator queryIter = _queries.begin();
				 queryIter != _queries.end();
				 queryIter++) {
			(*queryIter)->changed(_remoteAds[remoteAd]);
		}
	} else {
		// a new one
		EndPoint endPoint(boost::shared_ptr<EndPointImpl>(new EndPointImpl()));
		
		endPoint.getImpl()->setDomain(remoteAd->domain);
		endPoint.getImpl()->setHost(remoteAd->host);
		endPoint.getImpl()->setInProcess(remoteAd->isInProcess); // figure out
		
		// not sure what to do if we have multiple ips
		endPoint.getImpl()->setIP(remoteAd->ipv4.begin()->second);
		
		endPoint.getImpl()->setLastSeen(Thread::getTimeStampMs());
		endPoint.getImpl()->setPort(remoteAd->port);
		endPoint.getImpl()->setRemote(remoteAd->isRemote); // figure out
		
		endPoint.getImpl()->setTransport(remoteAd->getTransport());

		UM_LOG_INFO("MDNS reported new node %s in %s", endPoint.getAddress().c_str(), _config["mdns.domain"].c_str());

		_remoteAds[remoteAd] = endPoint;
			
		for (std::set<ResultSet<EndPoint>*>::iterator queryIter = _queries.begin();
				 queryIter != _queries.end();
				 queryIter++) {
			(*queryIter)->added(endPoint);
		}
	}
}

void MDNSDiscovery::removed(MDNSAd* remoteAd) {
	ScopeLock lock(_mutex);

	if(_remoteAds.find(remoteAd) == _remoteAds.end()) {
		UM_LOG_WARN("MDNS reported vanishing of unknown node %s in %s", remoteAd->host.c_str(), _config["mdns.domain"].c_str());
		return;
	}

	UM_LOG_INFO("MDNS reported vanished node %s in %s", _remoteAds[remoteAd].getAddress().c_str(), _config["mdns.domain"].c_str());

	for (std::set<ResultSet<EndPoint>*>::iterator queryIter = _queries.begin();
			 queryIter != _queries.end();
			 queryIter++) {
		(*queryIter)->removed(_remoteAds[remoteAd]);
	}
	_remoteAds.erase(remoteAd);
}

void MDNSDiscovery::changed(MDNSAd* remoteAd) {
	ScopeLock lock(_mutex);

	assert(_remoteAds.find(remoteAd) != _remoteAds.end());
	
	UM_LOG_INFO("MDNS reported changed node %s in %s", _remoteAds[remoteAd].getAddress().c_str(), _config["mdns.domain"].c_str());

	for (std::set<ResultSet<EndPoint>*>::iterator queryIter = _queries.begin();
			 queryIter != _queries.end();
			 queryIter++) {
		(*queryIter)->changed(_remoteAds[remoteAd]);
	}
}

std::vector<EndPoint> MDNSDiscovery::list() {
	ScopeLock lock(_mutex);

	std::vector<EndPoint> endpoints;
	for (std::map<MDNSAd*, EndPoint>::iterator adIter = _remoteAds.begin();
			 adIter != _remoteAds.end();
			 adIter++) {
		endpoints.push_back(adIter->second);
	}

	return endpoints;
}

}