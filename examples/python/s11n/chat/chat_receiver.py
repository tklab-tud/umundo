#!/usr/bin/env python2.7
# ^^ make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("../../../../build/lib") # set to wherever your umundo libraries are
sys.path.append("./generated/") # compiled protobuf
sys.path.append("../../../../s11n/src/umundo-python") # s11n python binding
	
import umundoS11n
from ChatS11N_pb2 import ChatMsg

from umundoS11n import umundo_proto as umundo


class TestReceiver(umundoS11n.TypedReceiver):
    def receive(self, *args):
        print("i")
        
    def receiveObject(self, *args):
    	print("blubber")

testRcv = TestReceiver()
chatSub = umundoS11n.TypedSubscriber("s11nChat", testRcv)

node = umundo.Node()
node.addSubscriber(chatSub)

while True:
	time.sleep(10)