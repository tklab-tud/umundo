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

#ifndef MULTICASTDNSDISCOVERY_H_P5FY838U
#define MULTICASTDNSDISCOVERY_H_P5FY838U

#include "umundo/discovery/Discovery.h"

namespace umundo {

class MDNSAd {
public:
	MDNSAd() : port(0), isInProcess(false), isRemote(true) {}
	std::string regType;
	std::string domain;
	std::string name;
	uint16_t port;
	std::string host;
	bool isInProcess;
	bool isRemote;
	std::set<std::string> txtRecord;
	std::set<uint32_t> interfaces;
	std::map<uint32_t, std::string> ipv4;
	std::map<uint32_t, std::string> ipv6;

	std::string getTransport() {
		size_t firstDot = regType.find_first_of(".");
		if (firstDot != std::string::npos && regType.length() >= firstDot + 5) {
			return regType.substr(firstDot + 2, 3);
		}
		return "";
	}

	std::string getServiceType() {
		size_t firstDot = regType.find_first_of(".");
		if (firstDot != std::string::npos) {
			return regType.substr(0, firstDot);
		}
		return "";
	}
};

class MDNSQuery {
public:
	std::string regType;
	std::string domain;
	ResultSet<MDNSAd*>* rs;
};

/**
 * Interface implemented by the actual mdns implementation.
 *
 * This is used by MDNSDiscovery objects to perform their implied queries.
 */
class MDNSDiscoveryImpl : public Implementation {
public:
	virtual void advertise(MDNSAd* node) = 0;
	virtual void unadvertise(MDNSAd* node) = 0;

	virtual void browse(MDNSQuery* query) = 0;
	virtual void unbrowse(MDNSQuery* query) = 0;
};

/**
 * Abstract base class for concrete implementations in the Factory.
 *
 * Concrete implementors are registered at program initialization at the Factory and
 * instantiated for every Abstraction that needs an implementation.
 */
class MDNSDiscovery : public DiscoveryImpl, public ResultSet<MDNSAd*> {
public:
	MDNSDiscovery();
	SharedPtr<Implementation> create();
	virtual ~MDNSDiscovery();

	void suspend();
	void resume();

	void advertise(const EndPoint& node);
	void add(Node& node);
	void unadvertise(const EndPoint& node);
	void remove(Node& node);

	void browse(ResultSet<EndPoint>* query);
	void unbrowse(ResultSet<EndPoint>* query);

	void added(MDNSAd*);
	void removed(MDNSAd*);
	void changed(MDNSAd*);

	std::vector<EndPoint> list();

protected:
	void init(const Options*);

	std::map<std::string, std::string> _config;
	MDNSQuery* _query;
	std::map<EndPoint, MDNSAd*> _localAds;
	std::map<MDNSAd*, EndPoint> _remoteAds;
	std::set<ResultSet<EndPoint>*> _queries;

	RMutex _mutex;
	SharedPtr<MDNSDiscoveryImpl> _mdnsImpl;
};

}

#endif /* end of include guard: MULTICASTDNSDISCOVERY_H_P5FY838U */
