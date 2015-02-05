/**
 *  @file
 *  @brief      An endpoint in IP networks.
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

#ifndef ENDPOINT_H_2O8TQUBP
#define ENDPOINT_H_2O8TQUBP

#include "umundo/core/Common.h"
#include "Implementation.h"
#include <boost/shared_ptr.hpp>

#define SHORT_UUID(uuid) (uuid.length() == 36 ? uuid.substr(0, 8) : uuid)

// cannot use reference with MSVC
#define ENDPOINT_RS_TYPE const EndPoint

namespace umundo {

class UMUNDO_API EndPointImpl {
public:
	EndPointImpl() :
		_port(0),
		_isRemote(true),
		_isInProcess(false),
		_lastSeen(Thread::getTimeStampMs()) {}

	virtual ~EndPointImpl() {}

	// STL container support and equality
	bool operator< (const EndPointImpl& other) const {
		if (this->_uuid.size() > 0 || other._uuid.size() > 0) {
			return this->_uuid < other._uuid;
		}
		return getAddress() < other.getAddress();
	}

	bool operator==(const EndPointImpl& other) const {
		return !(*this != other);
	}
	bool operator!=(const EndPointImpl& other) const {
		return (*this < other) || (other < *this);
	}

	virtual const std::string getAddress() const            {
		return _transport + "://" + _ip + ":" + toStr(_port) + (_uuid.size() > 0 ? " (" + SHORT_UUID(_uuid) + ")" : "");
	}
	virtual const std::string getIP() const            {
		return _ip;
	}
	virtual void setIP(std::string ip)                 {
		_ip = ip;
	}
	virtual const std::string getTransport() const     {
		return _transport;
	}
	virtual void setTransport(std::string transport)   {
		_transport = transport;
	}
	virtual uint16_t getPort() const              {
		return _port;
	}
	virtual void setPort(uint16_t port)           {
		_port = port;
	}
	virtual bool isRemote() const                 {
		return _isRemote;
	}
	virtual void setRemote(bool isRemote)         {
		_isRemote = isRemote;
	}
	virtual bool isInProcess() const              {
		return _isInProcess;
	}
	virtual void setInProcess(bool isInProcess)   {
		_isInProcess = isInProcess;
	}
	virtual const std::string getHost() const          {
		return _host;
	}
	virtual void setHost(std::string host)             {
		_host = host;
	}
	virtual const std::string getDomain() const        {
		return _domain;
	}
	virtual void setDomain(std::string domain)         {
		_domain = domain;
	}
	virtual const uint64_t getLastSeen() const        {
		return _lastSeen;
	}
	virtual void setLastSeen(uint64_t lastSeen)       {
		_lastSeen = lastSeen;
	}
	virtual void updateLastSeen() {
		_lastSeen = Thread::getTimeStampMs();
	}
	virtual std::string getUUID() const            {
		return _uuid;
	}
	virtual void setUUID(const std::string& uuid) {
		_uuid = uuid;
	}

	uint16_t implType; // this ought to be in the Implementation class. but class hierarchy won't fit

protected:
	std::string _ip;
	std::string _transport;
	uint16_t _port;
	bool _isRemote;
	bool _isInProcess;
	std::string _host;
	std::string _domain;
	uint64_t _lastSeen;
	std::string _uuid; // defaults to empty uuid

};

class UMUNDO_API EndPointConfig : public Options {
public:
	enum Protocol {
		TCP, UDP
	};

	EndPointConfig(uint16_t port) {
		setPort(port);
		options["endpoint.ip"] = "*";
	}

	EndPointConfig() {
		options["endpoint.ip"] = "*";
	}

	EndPointConfig(const std::string& address) {
		size_t colonPos = address.find_last_of(":");
		if(colonPos == std::string::npos || address.length() < 6 + 8 + 1 + 1) {
			UM_LOG_ERR("Endpoint address '%s' invalid", address.c_str());
			return;
		}

		std::string transport = address.substr(0,3);
		std::string ip = address.substr(6, colonPos - 6);
		std::string port = address.substr(colonPos + 1, address.length() - colonPos + 1);

		if (transport != "tcp") {
			UM_LOG_ERR("Endpoint transport '%s' invalid", transport.c_str());
			return;
		}

		if (!isNumeric(port.c_str(), 10) ||
		        port.find("-") != std::string::npos ||
		        port.find(".") != std::string::npos) {
			UM_LOG_ERR("Endpoint port '%s' invalid", port.c_str());
			return;
		}

		setPort(strTo<uint16_t>(port));
		setIP(ip);
	}

	std::string getType() {
		return "EndPointConfig";
	}

	void setPort(uint16_t nodePort) {
		options["endpoint.port"] = toStr(nodePort);
	}

	void setIP(const std::string& nodeIp) {
		options["endpoint.ip"] = toStr(nodeIp);
	}

};

/**
 * Anything that is addressable in TCP/IP networks.
 */
class UMUNDO_API EndPoint {
public:

	/**
	 * Convinience constructor for tcp://1.2.3.4:8080 style addresses
	 */
	EndPoint(const std::string& address) {
		size_t colonPos = address.find_last_of(":");
		if(colonPos == std::string::npos || address.length() < 6 + 8 + 1 + 1) {
			UM_LOG_ERR("Endpoint address '%s' invalid", address.c_str());
			return;
		}

		std::string transport = address.substr(0,3);
		std::string ip = address.substr(6, colonPos - 6);
		std::string port = address.substr(colonPos + 1, address.length() - colonPos + 1);

		if (transport != "tcp") {
			UM_LOG_ERR("Endpoint transport '%s' invalid", transport.c_str());
			return;
		}

		if (!isNumeric(port.c_str(), 10) ||
		        port.find("-") != std::string::npos ||
		        port.find(".") != std::string::npos) {
			UM_LOG_ERR("Endpoint port '%s' invalid", port.c_str());
			return;
		}

		_impl = SharedPtr<EndPointImpl>(new EndPointImpl());
		_impl->setTransport(transport);
		_impl->setIP(ip);
		_impl->setPort(strTo<uint16_t>(port));
	}

	EndPoint() : _impl() { }
	EndPoint(SharedPtr<EndPointImpl> const impl) : _impl(impl) { }
	EndPoint(const EndPoint& other) : _impl(other._impl) { }
	virtual ~EndPoint() { }

	operator bool() const {
		return _impl.get();
	}
	bool operator==(const EndPoint& other) const {
		return *_impl.get() == *other._impl.get();
	}
	bool operator!=(const EndPoint& other) const {
		return *_impl.get() != *other._impl.get();
	}
	bool operator<(const EndPoint& other) const {
		// use operator< from pointee
		return *_impl.get() < *other._impl.get();
	}

	EndPoint& operator=(const EndPoint& other) {
		_impl = other._impl;
		return *this;
	} // operator=

	virtual std::string getAddress() const {
		return _impl->getAddress();
	}

	virtual const std::string getIP() const {
		return _impl->getIP();
	}
	virtual const std::string getTransport() const {
		return _impl->getTransport();
	}
	virtual uint16_t getPort() const {
		return _impl->getPort();
	}
	virtual bool isRemote() const {
		return _impl->isRemote();
	}
	virtual bool isInProcess() const {
		return _impl->isInProcess();
	}
	virtual const std::string getHost() const {
		return _impl->getHost();
	}
	virtual const std::string getDomain() const {
		return _impl->getDomain();
	}
	virtual const uint64_t getLastSeen() const {
		return _impl->getLastSeen();
	}
	virtual const std::string getUUID() const {
		return _impl->getUUID();
	}
	virtual void updateLastSeen() {
		return _impl->updateLastSeen();
	}

	SharedPtr<EndPointImpl> getImpl() const {
		return _impl;
	}

protected:
	SharedPtr<EndPointImpl> _impl;
	friend std::ostream& operator<< (std::ostream& os, const EndPoint& endPoint);

};

}


#endif /* end of include guard: ENDPOINT_H_2O8TQUBP */
