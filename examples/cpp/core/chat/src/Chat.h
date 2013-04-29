#ifndef CHAT_H_C0YBZSDX
#define CHAT_H_C0YBZSDX

#include <umundo/core.h>
#include <map>

using namespace umundo;

class ChatReceiver : public Receiver {
public:
	ChatReceiver();
	virtual ~ChatReceiver() {}
	void receive(Message* msg);
};

class ChatGreeter : public Greeter {
public:
	ChatGreeter(const std::string& username, const std::string& subId);
	virtual ~ChatGreeter() {}
	virtual void welcome(Publisher, const string& nodeId, const string& subId);
	virtual void farewell(Publisher, const string& nodeId, const string& subId);
	
	std::string _username;
	std::string _subId;
};

#endif /* end of include guard: CHAT_H_C0YBZSDX */
