#ifndef CHAT_H_C0YBZSDX
#define CHAT_H_C0YBZSDX

#include <umundo/core.h>
#include <umundo/s11n.h>
#include "ChatS11N.pb.h"
#include "ChatS11N.rpc.pb.h"
#include <map>

using namespace umundo;

class ChatServiceImpl : public ChatServiceBase {
public:
	ChatServiceImpl();
	virtual ~ChatServiceImpl() {}
	virtual Void* receive(ChatMsg*);
	virtual Void* join(ChatMsg*);
	virtual Void* leave(ChatMsg*);
};

class ChatServiceListener : public ResultSet<ServiceDescription> {
public:
	ChatServiceListener();
	virtual ~ChatServiceListener() {}
	virtual void added(shared_ptr<ServiceDescription>);
	virtual void removed(shared_ptr<ServiceDescription>);
	virtual void changed(shared_ptr<ServiceDescription>);
};

#endif /* end of include guard: CHAT_H_C0YBZSDX */
