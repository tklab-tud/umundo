#!/usr/bin/python
# ^^ make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("../../../../build/lib") # set to wherever your umundo libraries are
sys.path.append("./generated/") # compiled protobuf
sys.path.append("../../../../s11n/src/umundo-python") # s11n python binding
try:
	import umundo64 as umundo
except ImportError:
	import umundo
import umundoS11n
from ChatS11N_pb2 import ChatMsg

class ChatReceiver(umundoS11n.TypedReceiver):
	def __init__(self, participants):
		self.participants = participants;

	def receiveObject(chatMsg, msg):
		if not object  is None:
			if chatMsg.type == ChatMsg.Type.JOINED:
				participants[msg.getMeta("subscriber")] = chatMsg.username
				print "%s joined the chat" % chatMsg.username
			elif chatMsg.type == ChatMsg.Type.NORMAL:
				print "%s: %s" % (chatMsg.username, chatMsg.message)

class ChatGreeter(umundoS11n.TypedReceiver):
	def __init__(self, publisher, subscriber, username, participants):
		self.publisher = publisher
		self.subscriber = subscriber
		self.username = username
		self.participants = participants

	def welcome(self, publisher, nodeId, subId):
		welcomeMsg = ChatMsg()
		welcomeMsg.username = self.userName
		welcomeMsg.type = ChatMsg.Type.JOINED
		greeting = self.publisher.prepareMessage("ChatMsg", welcomeMsg)
		greeting.setReceiver(subId)
		greeting.putMeta("subscriber", self.subscriber.getUUID())
		self.publisher.send(greeting)

	def farewell(self, publisher, nodeId, subId):
		if subId in participants:
			print "%s left the chat" % subId
		else:
			print "An unknown user left the chat: %s" % subId


username = raw_input("Dein Username: ")
participants = {}

chatRcv = ChatReceiver(participants);
chatSub = umundoS11n.TypedSubscriber("s11nChat", chatRcv);
chatPub = umundoS11n.TypedPublisher("s11nChat")
chatGrt = ChatGreeter(chatPub, chatSub, username, participants)

chatSub.registerType(ChatMsg())
chatPub.setGreeter(chatGrt)

node = umundo.Node()
node.addPublisher(chatPub)
node.addSubscriber(chatSub)

print("Start typing messages (empty line to quit):")

while True:
	inputmsg = raw_input("")
	if inputmsg == "":
		node.removePublisher(chatPub)
		node.removeSubscriber(chatSub)
		break

	msg = ChatMsg()
	msg.username = username
	msg.message = inputmsg
	chatPub.sendObject("ChatMsg", msg);