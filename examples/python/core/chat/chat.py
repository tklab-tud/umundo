#!/usr/bin/python
# ^^ make sure the interpreter is the one used while building!

import sys
# set to wherever your umundo libraries are
sys.path.append("../../../../build/cli/lib")

import time
import umundo64

node = umundo64.Node();
pub = umundo64.Publisher("chat");
sub = umundo64.Subscriber("chat");

# TODO: actually implement the chat
time.sleep(10);