/**
 *  @file
 *  @brief      Message class for all data that is sent.
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

#ifndef MESSAGE_H_Y7TB6U8
#define MESSAGE_H_Y7TB6U8

#include "umundo/common/Common.h"
#include <string.h>

namespace umundo {
class Message;
class Type;

/**
 * Definition of message types and abstraction of message (bridge pattern).
 */
class DLLEXPORT Message {
public:
	enum Type {
		VERSION            = 0xF004, // version 0.4 of the message format
		CONNECT_REQ        = 0x0001, // sent to a remote node when it was added
		CONNECT_REP        = 0x0002, // reply from a remote node
		NODE_INFO          = 0x0003, // information about a node and its publishers
		PUB_ADDED          = 0x0004, // sent when a node added a publisher
		PUB_REMOVED        = 0x0005, // sent when a node removed a publisher
		SUB_ADDED          = 0x000D, // sent when a node added a subscriber
		SUB_REMOVED        = 0x000E, // sent when a node removed a subscriber
		SUBSCRIBE          = 0x0006, // sent when subscribing to a publisher
		UNSUBSCRIBE        = 0x0007, // unsusbscribing from a publisher
		DISCONNECT         = 0x0008, // node was removed
		DEBUG              = 0x0009, // request debug info
		ZMQ_UPDATE_SOCKETS = 0xA001, // zmq specific internal message
		SHUTDOWN           = 0x000C, // node is shutting down
	};

	enum Flags { // not used yet
	    NONE            = 0x0000,
	    ADOPT_DATA      = 0x0001,
	    ZERO_COPY       = 0x0002,
	};

	static const char* typeToString(uint16_t type) {
		if (type == VERSION)            return "VERSION";
		if (type == CONNECT_REQ)        return "CONNECT_REQ";
		if (type == CONNECT_REP)        return "CONNECT_REP";
		if (type == NODE_INFO)          return "NODE_INFO";
		if (type == PUB_ADDED)          return "PUB_ADDED";
		if (type == PUB_REMOVED)        return "PUB_REMOVED";
		if (type == SUB_ADDED)          return "SUB_ADDED";
		if (type == SUB_REMOVED)        return "SUB_REMOVED";
		if (type == SUBSCRIBE)          return "SUBSCRIBE";
		if (type == UNSUBSCRIBE)        return "UNSUBSCRIBE";
		if (type == DISCONNECT)         return "DISCONNECT";
		if (type == DEBUG)              return "DEBUG";
		if (type == ZMQ_UPDATE_SOCKETS) return "ZMQ_UPDATE_SOCKETS";
		if (type == SHUTDOWN)           return "SHUTDOWN";
		return "UNKNOWN";
	}

	Message() : _data(NULL), _size(0), _isQueued(false) {}
	Message(const char* data, size_t length, Flags flag = NONE) : _data(NULL), _size(length), _isQueued(false), _flags(flag) {
		if (_size > 0) {
			_data = (char*)malloc(_size);
			memcpy(_data, data, _size);
		}
	}

	Message(const Message& other) : _data(NULL), _size(other.size()), _isQueued(other._isQueued) {
		if (_size > 0) {
			_data = (char*)malloc(_size);
			memcpy(_data, other.data(), _size);
		}
		// STL containers will copy themselves
		_meta = other._meta;
	}

	virtual ~Message() {
		if (_data)
			free(_data);
	}

	virtual const char* data() const                                    {
		return _data;
	}
	virtual size_t size() const                                         {
		return _size;
	}

	virtual void setData(const char* data, size_t length)               {
		if (_data)
			free(_data);
		_size = length;
		_data = (char*)malloc(_size);
		memcpy(_data, data, _size);
	}
	virtual const void putMeta(const std::string& key, const std::string& value)  {
		_meta[key] = value;
	}
	virtual const std::map<std::string, std::string>& getMeta()                        {
		return _meta;
	}
	virtual const std::string getMeta(const std::string& key)                    {
		if (_meta.find(key) != _meta.end())
			return _meta[key];
		return "";
	}

#if 0
	/// Simplified access to keyset for Java, namespace qualifiers required for swig!
	virtual const std::vector<std::string> getKeys() {
		std::vector<std::string> keys;
		std::map<std::string, std::string>::const_iterator metaIter = _meta.begin();
		while (metaIter != _meta.end()) {
			keys.push_back(metaIter->first);
			metaIter++;
		}
		return keys;
	}
#endif

	void setQueued(bool isQueued) {
		_isQueued = isQueued;
	}
	bool isQueued() {
		return _isQueued;
	}

	void setReceiver(const std::string& uuid) {
		putMeta("um.sub", uuid);
	}

	static Message* toSubscriber(const std::string& uuid) {
		Message* msg = new Message();
		msg->putMeta("um.sub", uuid);
		return msg;
	}

protected:
	char* _data;
	size_t _size;
	bool _isQueued;
	uint32_t _flags;
	std::map<std::string, std::string> _meta;
};
}


#endif /* end of include guard: MESSAGE_H_Y7TB6U8 */
