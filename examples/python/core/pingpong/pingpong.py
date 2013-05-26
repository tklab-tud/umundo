#!/usr/bin/python
# make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("../../../../build/lib") # set to wherever your umundo libraries are
try:
    import umundo64 as umundo
except ImportError:
    import umundo


class TestReceiver(umundo.Receiver):
    def receive(self, *args):
        sys.stdout.write("i")
        sys.stdout.flush()

testRcv = TestReceiver()
pub = umundo.Publisher("pingpong")
sub = umundo.Subscriber("pingpong", testRcv)

node = umundo.Node()
node.addPublisher(pub)
node.addSubscriber(sub)

while True:
    time.sleep(1)
    msg = umundo.Message()
    msg.setData("ping")
    pub.send(msg)

    sys.stdout.write("o")
    sys.stdout.flush()
