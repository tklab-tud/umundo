/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
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
	    DATA          = 0x0000,
	    PUB_ADDED     = 0x0004,
	    PUB_REMOVED   = 0x0005,
	    NODE_INFO     = 0x0006,
	    SUBSCRIBE     = 0x0007,
	    UNSUBSCRIBE   = 0x0008,
	};

	static const char* typeToString(uint16_t type) {
		if (type == 0x0000) return "DATA";
		if (type == 0x0004) return "PUB_ADDED";
		if (type == 0x0005) return "PUB_REMOVED";
		if (type == 0x0006) return "NODE_INFO";
		if (type == 0x0007) return "SUBSCRIBE";
		if (type == 0x0008) return "UNSUBSCRIBE";
		return "UNKNOWN";
	}

	Message() : _data(NULL), _size(0) {}
	Message(const char* data, size_t length) : _data(NULL), _size(length) {
		if (_size > 0) {
			_data = (char*)malloc(_size);
			memcpy(_data, data, _size);
		}
	}

	Message(const Message& other) : _data(NULL), _size(other.size()) {
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
	virtual void setData(const string& data)                            {
		if (_data)
			free(_data);
		_size = data.size();
		_data = (char*)malloc(_size);
		memcpy(_data, data.data(), _size);
	}
	virtual void setData(const char* data, size_t length)               {
		if (_data)
			free(_data);
		_size = length;
		_data = (char*)malloc(_size);
		memcpy(_data, data, _size);
	}
	virtual const void putMeta(const string& key, const string& value)  {
		_meta[key] = value;
	}
	virtual const map<string, string>& getMeta()                        {
		return _meta;
	}
	virtual const string& getMeta(const string& key)                    {
		return _meta[key];
	}

	/// Simplified access to keyset for Java, namespace qualifiers required for swig!
	virtual const std::vector<std::string> getKeys() {
		vector<string> keys;
		map<string, string>::const_iterator metaIter = _meta.begin();
		while (metaIter != _meta.end()) {
			keys.push_back(metaIter->first);
			metaIter++;
		}
		return keys;
	}

	static Message* toSubscriber(const string& uuid) {
		Message* msg = new Message();
		msg->putMeta("subscriber", uuid);
		return msg;
	}

protected:
	char* _data;
	size_t _size;
	map<string, string> _meta;
};
}


#endif /* end of include guard: MESSAGE_H_Y7TB6U8 */
