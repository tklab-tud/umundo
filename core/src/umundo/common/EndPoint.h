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

namespace umundo {
/**
 * Anything that is addressable in TCP/IP networks.
 */
class DLLEXPORT EndPoint {
public:
	virtual ~EndPoint() {}

	virtual const string& getIP() const           {
		return _ip;
	}
	virtual void setIP(string ip)                 {
		_ip = ip;
	}
	virtual const string& getTransport() const    {
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
	virtual void setInProcess(bool isInProcess) {
		_isInProcess = _isInProcess;
	}
	virtual const string& getHost() const         {
		return _host;
	}
	virtual void setHost(string host)             {
		_host = host;
	}
	virtual const string& getDomain() const       {
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
}


#endif /* end of include guard: ENDPOINT_H_2O8TQUBP */
