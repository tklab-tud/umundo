#!/usr/bin/env python2.7
# make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("./generated/") # compiled protobuf
sys.path.append("../../../../s11n/src/umundo-python") # s11n python binding

import umundoS11n
from umundoS11n import umundo_proto as umundo
from ChatS11N_pb2 import ChatMsg

class ChatReceiver(umundoS11n.TypedReceiver):
    def __init__(self, participants):
        super(ChatReceiver,self).__init__()
        self.participants = participants;

    def receiveObject(self, chatMsg, msg):
        try:
            if not msg is None:
                if chatMsg.type == ChatMsg.JOINED:
                    participants[msg.getMeta("subscriber")] = chatMsg.username
                    print "%s joined the chat" % chatMsg.username
                elif chatMsg.type == ChatMsg.NORMAL:
                    print "%s: %s" % (chatMsg.username, chatMsg.message)
        except BaseException as e:
            traceback.print_exc(file=sys.stdout)


class ChatGreeter(umundo.Greeter):
    def __init__(self, publisher, subscriber, username, participants):
        super(ChatGreeter,self).__init__()
        self.publisher = publisher
        self.subscriber = subscriber
        self.username = username
        self.participants = participants

    def welcome(self, publisher, nodeId, subId):
        try:
            welcomeMsg = ChatMsg()
            welcomeMsg.username = self.username
            welcomeMsg.type = ChatMsg.JOINED

            greeting = self.publisher.prepareMessage("ChatMsg", welcomeMsg)
            greeting.setReceiver(subId)
            greeting.putMeta("subscriber", self.subscriber.getUUID())
            self.publisher.send(greeting)
        except BaseException as e:
            traceback.print_exc(file=sys.stdout)

    def farewell(self, publisher, nodeId, subId):
        try:
            if subId in participants:
                print "%s left the chat" % subId
            else:
                print "An unknown user left the chat: %s" % subId
        except BaseException as e:
            traceback.print_exc(file=sys.stdout)


username = raw_input("Your username: ")
participants = {}

chatRcv = ChatReceiver(participants)
chatSub = umundoS11n.TypedSubscriber("s11nChat", chatRcv)
chatPub = umundoS11n.TypedPublisher("s11nChat")
chatGrt = ChatGreeter(chatPub, chatSub, username, participants)

chatSub.registerType("ChatMsg", ChatMsg)
#chatPub.setGreeter(chatGrt)

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
    chatPub.sendObject("ChatMsg", msg)
