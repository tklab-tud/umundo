#include "Chat.h"

std::map<std::string, std::string> _participants;

ChatReceiver::ChatReceiver() {}

void ChatReceiver::receive(void* obj, Message* msg) {
	if (obj != NULL) {
		ChatMsg* chatMsg = (ChatMsg*)obj;

		if (chatMsg->type() == ChatMsg::JOINED) {
			_participants[msg->getMeta("subscriber")] = chatMsg->username();
			std::cout << chatMsg->username() << " joined the chat" << std::endl;
		} else if (chatMsg->type() == ChatMsg::NORMAL) {
			std::cout << chatMsg->username() << ": " << chatMsg->message() << std::endl;
		}
	}
}

ChatGreeter::ChatGreeter(const std::string& username, const std::string& subId) : _username(username), _subId(subId) {}

void ChatGreeter::welcome(TypedPublisher atPub, const umundo::SubscriberStub& sub) {
	ChatMsg* welcomeMsg = new ChatMsg();
	welcomeMsg->set_type(ChatMsg::JOINED);
	welcomeMsg->set_username(_username.c_str());

	Message* greeting = atPub.prepareMsg("ChatMsg", welcomeMsg);
	greeting->setReceiver(sub.getUUID());

	atPub.send(greeting);
	delete greeting;
	delete welcomeMsg;
}

void ChatGreeter::farewell(TypedPublisher fromPub, const umundo::SubscriberStub& sub) {
	if (_participants.find(sub.getUUID()) != _participants.end()) {
		std::cout << _participants[sub.getUUID()] << " left the chat" << std::endl;
		_participants.erase(sub.getUUID());
	} else {
		std::cout << "An unknown user left the chat: " << std::endl;	
	}
}

int main(int argc, char** argv) {
	std::string username;
	std::cout << "Username:" << std::endl;
	std::getline(std::cin, username);

	Node chatNode;
  Discovery disc(Discovery::MDNS);
  disc.add(chatNode);
  
	TypedSubscriber chatSub("s11nChat", new ChatReceiver());
	TypedPublisher chatPub("s11nChat");
	chatPub.setGreeter(new ChatGreeter(username, chatSub.getUUID()));
	
	chatSub.registerType("ChatMsg", new ChatMsg());
	
	chatNode.addPublisher(chatPub);
	chatNode.addSubscriber(chatSub);
	
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
		chatPub.sendObj("ChatMsg", chatMsg);
		delete chatMsg;
	}
	
	return 0;
}