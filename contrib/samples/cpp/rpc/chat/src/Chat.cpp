#include "Chat.h"

std::map<std::string, std::string> _participants;
std::map<ServiceDescription*, ChatServiceStub*> _remoteChatServices;
std::string username;

ChatServiceImpl::ChatServiceImpl() {
}

Void* ChatServiceImpl::receive(ChatMsg* msg) {
	std::cout << msg->username() << ": " << msg->message() << std::endl;
	return new Void();
}

Void* ChatServiceImpl::join(ChatMsg* msg) {
	std::cout << msg->username() << " joined the chat" << std::endl;
	return new Void();
}

Void* ChatServiceImpl::leave(ChatMsg* msg) {
	std::cout << msg->username() << " left the chat" << std::endl;
	return new Void();
}

ChatServiceListener::ChatServiceListener() {
	
}

void ChatServiceListener::added(shared_ptr<ServiceDescription> svcDesc) {
	ChatServiceStub* remoteChatSvc = new ChatServiceStub(svcDesc.get());
	ChatMsg* joinMsg = new ChatMsg();
	joinMsg->set_username(username.c_str());
	remoteChatSvc->join(joinMsg);
	delete joinMsg;
	
	_remoteChatServices[svcDesc.get()] = remoteChatSvc;
}

void ChatServiceListener::removed(shared_ptr<ServiceDescription> svcDesc) {
	_remoteChatServices.erase(svcDesc.get());
}

void ChatServiceListener::changed(shared_ptr<ServiceDescription> svcDesc) {
	
}

int main(int argc, char** argv) {
	std::cout << "Username:" << std::endl;
	std::getline(std::cin, username);

	Node* chatNode = new Node();
	ServiceManager* svcMgr = new ServiceManager();
	ChatServiceImpl* localChatService = new ChatServiceImpl();
	svcMgr->addService(localChatService);
	chatNode->connect(svcMgr);
	
	ServiceFilter* svcFilter = new ServiceFilter("ChatService");
	svcMgr->startQuery(svcFilter, new ChatServiceListener());
	
	std::string line;
	std::cout << "Start typing messages (empty line to quit):" << std::endl;
	while(true) {
		std::string line;
		std::getline(std::cin, line);
		if (line.length() == 0)
			break;

		ChatMsg* chatMsg = new ChatMsg();
		chatMsg->set_username(username.c_str());
		chatMsg->set_message(line.c_str());
		
		std::map<ServiceDescription*, ChatServiceStub*>::iterator svcIter = _remoteChatServices.begin();
		while(svcIter != _remoteChatServices.end()) {
			svcIter->second->receive(chatMsg);
			svcIter++;
		}
		delete chatMsg;
	}
	
	return 0;
}