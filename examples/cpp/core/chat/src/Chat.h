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
  virtual void welcome(Publisher& pub, const SubscriberStub& sub);
  virtual void farewell(Publisher& pub, const SubscriberStub& sub);
  
	std::string _username;
	std::string _subId;
};

#endif /* end of include guard: CHAT_H_C0YBZSDX */
