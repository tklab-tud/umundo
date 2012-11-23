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

#include "umundo/common/Common.h"
#include "Implementation.h"
#include <boost/shared_ptr.hpp>

namespace umundo {
	
	class DLLEXPORT EndPointImpl {
	public:
		virtual ~EndPointImpl() {}

		virtual const string getIP() const           {
			return _ip;
		}
		virtual void setIP(string ip)                 {
			_ip = ip;
		}
		virtual const string getTransport() const    {
			return _transport;
		}
		virtual void setTransport(string transport)   {
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
			_isInProcess = _isInProcess;
		}
		virtual const string getHost() const         {
			return _host;
		}
		virtual void setHost(string host)             {
			_host = host;
		}
		virtual const string getDomain() const       {
			return _domain;
		}
		virtual void setDomain(string domain)         {
			_domain = domain;
		}

	protected:
		string _ip;
		string _transport;
		uint16_t _port;
		bool _isRemote;
		bool _isInProcess;
		string _host;
		string _domain;
	};

/**
 * Anything that is addressable in TCP/IP networks.
 */
class DLLEXPORT EndPoint {
public:

  EndPoint() : _impl() { }
  EndPoint(boost::shared_ptr<EndPointImpl> const impl) : _impl(impl) { }
  EndPoint(const EndPoint& other) : _impl(other._impl) { }
  virtual ~EndPoint() { }

  operator bool() const { return _impl; }
  bool operator==(const EndPoint& other) const { return _impl == other._impl; }
  bool operator!=(const EndPoint& other) const { return _impl != other._impl; }

  EndPoint& operator=(const EndPoint& other) 
  {
    _impl = other._impl;
    return *this;
  } // operator=

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

protected:
	boost::shared_ptr<EndPointImpl> _impl;
};

}


#endif /* end of include guard: ENDPOINT_H_2O8TQUBP */
