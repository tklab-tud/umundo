#!/usr/bin/python
# ^^ make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("../../../build/cli/lib") # set to wherever your umundo libraries are
import umundo64

class TestReceiver(umundo64.Receiver):
    def receive(self, *args):
        sys.stdout.write("i")
        sys.stdout.flush()
        
print "umundo-pingpong version python\n"

testRcv = TestReceiver()
pub = umundo64.Publisher("pingpong")
sub = umundo64.Subscriber("pingpong", testRcv)
#sub = umundo64.Subscriber("pingpong")

node = umundo64.Node()
node.addPublisher(pub)
node.addSubscriber(sub)

while True:
    time.sleep(1)
    msg = umundo64.Message()
    pub.send(msg)

    sys.stdout.write("o")
    sys.stdout.flush()