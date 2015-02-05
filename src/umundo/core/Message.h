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
	/**
	 * On-wire message types
	 */
	enum Type {
		UM_VERSION            = 0xF005, // version 0.5 of the message format
		UM_CONNECT_REQ        = 0x0001, // sent to a remote node when it was added
		UM_CONNECT_REP        = 0x0002, // reply from a remote node
		UM_NODE_INFO          = 0x0003, // information about a node and its publishers (unused, CONNECT_REP for now)
		UM_PUB_ADDED          = 0x0004, // sent when a node added a publisher
		UM_PUB_REMOVED        = 0x0005, // sent when a node removed a publisher
		UM_SUBSCRIBE          = 0x0006, // sent when subscribing to a publisher
		UM_UNSUBSCRIBE        = 0x0007, // unsusbscribing from a publisher
		UM_DEBUG              = 0x0009, // request debug info
		UM_SHUTDOWN           = 0x000C, // node is shutting down
	};

	enum Flags {
		NONE            = 0x0000, ///< Default is to copy data and deallocate
		ADOPT_DATA      = 0x0001, ///< Do not copy into message but deallocate when done
		WRAP_DATA       = 0x0002, ///< Just wrap data, do not copy, do not deallocate
	};

	static const char* typeToString(uint16_t type) {
		if (type == UM_VERSION)            return "VERSION";
		if (type == UM_CONNECT_REQ)        return "CONNECT_REQ";
		if (type == UM_CONNECT_REP)        return "CONNECT_REP";
		if (type == UM_NODE_INFO)          return "NODE_INFO";
		if (type == UM_PUB_ADDED)          return "PUB_ADDED";
		if (type == UM_PUB_REMOVED)        return "PUB_REMOVED";
		if (type == UM_SUBSCRIBE)			     return "SUBSCRIBE";
		if (type == UM_UNSUBSCRIBE)        return "UNSUBSCRIBE";
		if (type == UM_DEBUG)              return "DEBUG";
		if (type == UM_SHUTDOWN)           return "SHUTDOWN";
		return "UNKNOWN";
	}

	static char* write(char* to, const std::string& value, bool terminate = true);
	static char* write(char* to, uint64_t value);
	static char* write(char* to, uint32_t value);
	static char* write(char* to, uint16_t value);
	static char* write(char* to, uint8_t value);
	static char* write(char* to, int64_t value);
	static char* write(char* to, int32_t value);
	static char* write(char* to, int16_t value);
	static char* write(char* to, int8_t value);
	static char* write(char* to, float value);
	static char* write(char* to, double value);

	static const char* read(const char* from, std::string& value, size_t maxLength);
	static const char* read(const char* from, uint64_t* value);
	static const char* read(const char* from, uint32_t* value);
	static const char* read(const char* from, uint16_t* value);
	static const char* read(const char* from, uint8_t* value);
	static const char* read(const char* from, int64_t* value);
	static const char* read(const char* from, int32_t* value);
	static const char* read(const char* from, int16_t* value);
	static const char* read(const char* from, int8_t* value);
	static const char* read(const char* from, float* value);
	static const char* read(const char* from, double* value);

private:
	class NilDeleter;
	friend class NilDeleter;
	friend class ZeroMQPublisher;

public:
	Message() : _size(0), _isQueued(false), _flags(NONE), _hint(NULL), _doneCallback(NULL) {}
	Message(const char* data, size_t length, Flags flags) : _size(length), _isQueued(false), _flags(flags), _hint(NULL), _doneCallback(NULL) {
		if (_flags & ADOPT_DATA) {
			// take ownership of data and delete when done
			_data = SharedPtr<char>(const_cast<char*>(data));
		} else if (_flags & WRAP_DATA) {
			// do not take ownership and do not delete when done
			_data = SharedPtr<char>(const_cast<char*>(data), Message::NilDeleter());
		} else {
			// copy into message
			_data = SharedPtr<char>((char*)malloc(_size));
			memcpy(_data.get(), data, _size);
		}
	}

	// need this one for SWIG to ignore the other one with flags
	Message(const char* data, size_t length) : _size(length), _isQueued(false), _flags(NONE), _hint(NULL), _doneCallback(NULL) {
		_data = SharedPtr<char>((char*)malloc(_size));
		memcpy(_data.get(), data, _size);
	}

	Message(const char* data, size_t length, void(*doneCallback)(void *data, void *hint), void* hint) : _size(length), _isQueued(false), _flags(WRAP_DATA) {
		_doneCallback = doneCallback;
		_hint = hint;
		_data = SharedPtr<char>(const_cast<char*>(data), Message::NilDeleter());
	}

	Message(const Message& other) : _size(other.size()), _isQueued(other._isQueued), _flags(other._flags) {
		_data = other._data;
		_meta = other._meta;
		_hint = other._hint;
		_doneCallback = other._doneCallback;
	}

	virtual ~Message() {
	}

	virtual const char* data() const                                    {
		return _data.get();
	}
	virtual size_t size() const                                         {
		return _size;
	}

	virtual uint32_t getFlags() const                                   {
		return _flags;
	}

	void compress();
	void uncompress();
	bool isCompressed() {
		return (_meta.find("um.compressed") != _meta.end());
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


private:
	class NilDeleter {
	public:
		void operator()(char* p) {}
	};

	SharedPtr<char> _data;
	size_t _size;

	bool _isQueued;
	uint32_t _flags;
	std::map<std::string, std::string> _meta;
	void* _hint;
	void (*_doneCallback) (void *data, void *hint);
};
}


#endif /* end of include guard: MESSAGE_H_Y7TB6U8 */
