#!/usr/bin/python
# make sure the interpreter is the one used while building!

import sys
import time

sys.path.append("../../../../build/lib") # set to wherever your umundo libraries are
try:
	import umundo64 as umundo
except ImportError:
	import umundo

import time

node = umundo.Node();
pub = umundo.Publisher("chat");
sub = umundo.Subscriber("chat");

# TODO: actually implement the chat
time.sleep(10);
