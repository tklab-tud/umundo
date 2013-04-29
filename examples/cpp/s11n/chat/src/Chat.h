#ifndef CHAT_H_C0YBZSDX
#define CHAT_H_C0YBZSDX

#include <umundo/core.h>
#include <umundo/s11n.h>
#include "ChatS11N.pb.h"
#include <map>

using namespace umundo;

class ChatReceiver : public TypedReceiver {
public:
	ChatReceiver();
	virtual ~ChatReceiver() {}
	void receive(void* obj, Message* msg);
};

class ChatGreeter : public TypedGreeter {
public:
	ChatGreeter(const std::string& username, const std::string& subId);
	virtual ~ChatGreeter() {}
	void welcome(TypedPublisher atPub, const std::string& nodeId, const std::string& subId);
	void farewell(TypedPublisher fromPub, const std::string& nodeId, const std::string& subId);
	
	std::string _username;
	std::string _subId;
};

#endif /* end of include guard: CHAT_H_C0YBZSDX */
