#include "Chat.h"

std::map<std::string, std::string> _participants;

ChatReceiver::ChatReceiver() {}

void ChatReceiver::receive(Message* msg) {
	if (msg->getMeta().find("participant") != msg->getMeta().end()) {
		_participants[msg->getMeta("subscriber")] = msg->getMeta("participant");
		std::cout << msg->getMeta("participant") << " joined the chat" << std::endl;
	} else {
		std::cout << msg->getMeta("userName") << ": " << msg->getMeta("chatMsg") << std::endl;
	}
}

ChatGreeter::ChatGreeter(const std::string& username, const std::string& subId) : _username(username), _subId(subId) {}

void ChatGreeter::welcome(Publisher& pub, const SubscriberStub& sub) {
	Message* greeting = Message::toSubscriber(sub.getUUID());
	greeting->putMeta("participant", _username);
	greeting->putMeta("subscriber", sub.getUUID());
	pub.send(greeting);
	delete greeting;
}

void ChatGreeter::farewell(Publisher& pub, const SubscriberStub& sub) {
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

	Discovery disc(Discovery::MDNS);
	Node chatNode;
	disc.add(chatNode);
	
	Subscriber chatSub("coreChat", new ChatReceiver());
	Publisher chatPub("coreChat");
	chatPub.setGreeter(new ChatGreeter(username, chatSub.getUUID()));
		
	chatNode.addPublisher(chatPub);
	chatNode.addSubscriber(chatSub);
	
	std::string line;
	std::cout << "Start typing messages (empty line to quit):" << std::endl;
	while(true) {
		std::string line;
		std::getline(std::cin, line);
		if (line.length() == 0)
			break;
		Message* msg = new Message();
		msg->putMeta("userName", username.c_str());
		msg->putMeta("chatMsg", line.c_str());
		chatPub.send(msg);
		delete msg;
	}
	
	return 0;
}