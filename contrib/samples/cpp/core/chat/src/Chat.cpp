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

void ChatGreeter::welcome(Publisher* atPub, const std::string& nodeId, const std::string& subId) {
	Message* greeting = Message::toSubscriber(subId);
	greeting->putMeta("participant", _username);
	greeting->putMeta("subscriber", _subId);
	atPub->send(greeting);
	delete greeting;
}

void ChatGreeter::farewell(Publisher* fromPub, const std::string& nodeId, const std::string& subId) {
	if (_participants.find(subId) != _participants.end()) {
		std::cout << _participants[subId] << " left the chat" << std::endl;
		_participants.erase(subId);
	} else {
		std::cout << "An unknown user left the chat: " << std::endl;	
	}
}

int main(int argc, char** argv) {
	std::string username;
	std::cout << "Username:" << std::endl;
	std::getline(std::cin, username);

	Node* chatNode = new Node();
	Subscriber* chatSub = new Subscriber("coreChat", new ChatReceiver());
	Publisher* chatPub = new Publisher("coreChat");
	chatPub->setGreeter(new ChatGreeter(username, chatSub->getUUID()));
		
	chatNode->addPublisher(chatPub);
	chatNode->addSubscriber(chatSub);
	
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
		chatPub->send(msg);
		delete msg;
	}
	
	return 0;
}