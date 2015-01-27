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

#include "umundo/core/Common.h"
#include <string.h>

namespace umundo {
class Message;
class Type;

/**
 * Definition of message types and abstraction of message (bridge pattern).
 */
class UMUNDO_API Message {
public:
	enum Type {
		UM_VERSION            = 0xF005, // version 0.5 of the message format
		UM_CONNECT_REQ        = 0x0001, // sent to a remote node when it was added
		UM_CONNECT_REP        = 0x0002, // reply from a remote node
		UM_NODE_INFO          = 0x0003, // information about a node and its publishers
		UM_PUB_ADDED          = 0x0004, // sent when a node added a publisher
		UM_PUB_REMOVED        = 0x0005, // sent when a node removed a publisher
		UM_SUB_ADDED          = 0x000D, // sent when a node added a subscriber
		UM_SUB_REMOVED        = 0x000E, // sent when a node removed a subscriber
		UM_SUBSCRIBE          = 0x0006, // sent when subscribing to a publisher
		UM_UNSUBSCRIBE        = 0x0007, // unsusbscribing from a publisher
		UM_DISCONNECT         = 0x0008, // node was removed
		UM_DEBUG              = 0x0009, // request debug info
		UM_ZMQ_UPDATE_SOCKETS = 0xA001, // zmq specific internal message
		UM_SHUTDOWN           = 0x000C, // node is shutting down
	};

	enum Flags { // not used yet
		NONE            = 0x0000,
		ADOPT_DATA      = 0x0001,
		ZERO_COPY       = 0x0002,
	};

	static const char* typeToString(uint16_t type) {
		if (type == UM_VERSION)            return "VERSION";
		if (type == UM_CONNECT_REQ)        return "CONNECT_REQ";
		if (type == UM_CONNECT_REP)        return "CONNECT_REP";
		if (type == UM_NODE_INFO)          return "NODE_INFO";
		if (type == UM_PUB_ADDED)          return "PUB_ADDED";
		if (type == UM_PUB_REMOVED)        return "PUB_REMOVED";
		if (type == UM_SUB_ADDED)          return "SUB_ADDED";
		if (type == UM_SUB_REMOVED)        return "SUB_REMOVED";
		if (type == UM_SUBSCRIBE)          return "SUBSCRIBE";
		if (type == UM_UNSUBSCRIBE)        return "UNSUBSCRIBE";
		if (type == UM_DISCONNECT)         return "DISCONNECT";
		if (type == UM_DEBUG)              return "DEBUG";
		if (type == UM_ZMQ_UPDATE_SOCKETS) return "ZMQ_UPDATE_SOCKETS";
		if (type == UM_SHUTDOWN)           return "SHUTDOWN";
		return "UNKNOWN";
	}

	static char* write(uint64_t value, char* to);
	static char* write(uint32_t value, char* to);
	static char* write(uint16_t value, char* to);
	static char* write(uint8_t value, char* to);
	static char* write(int64_t value, char* to);
	static char* write(int32_t value, char* to);
	static char* write(int16_t value, char* to);
	static char* write(int8_t value, char* to);
	static char* write(float value, char* to);
	static char* write(double value, char* to);
	
	static const char* read(uint64_t* value, const char* from);
	static const char* read(uint32_t* value, const char* from);
	static const char* read(uint16_t* value, const char* from);
	static const char* read(uint8_t* value, const char* from);
	static const char* read(int64_t* value, const char* from);
	static const char* read(int32_t* value, const char* from);
	static const char* read(int16_t* value, const char* from);
	static const char* read(int8_t* value, const char* from);
	static const char* read(float* value, const char* from);
	static const char* read(double* value, const char* from);

	Message() : _size(0), _isQueued(false) {}
	Message(const char* data, size_t length, Flags flag = NONE) : _size(length), _isQueued(false), _flags(flag) {
		_data = SharedPtr<char>((char*)malloc(_size));
		memcpy(_data.get(), data, _size);
	}

	Message(const Message& other) : _size(other.size()), _isQueued(other._isQueued) {
		_size = other._size;
		_data = other._data;
		_meta = other._meta;
	}

	virtual ~Message() {
	}

	virtual const char* data() const                                    {
		return _data.get();
	}
	virtual size_t size() const                                         {
		return _size;
	}

	virtual void setData(const char* data, size_t length)               {
		_size = length;
		_data = SharedPtr<char>((char*)malloc(_size));
		memcpy(_data.get(), data, _size);
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
	SharedPtr<char> _data;
	size_t _size;
	bool _isQueued;
	uint32_t _flags;
	std::map<std::string, std::string> _meta;
};
}


#endif /* end of include guard: MESSAGE_H_Y7TB6U8 */
