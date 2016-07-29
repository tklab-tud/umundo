#!/usr/bin/env python2.7
# make sure the interpreter is the one used while building!

# this is used to test the python core binding

import sys
import time

sys.path.append("../../../../build/cli-release/lib") # set to wherever your umundo libraries are
try:
    import umundo64 as umundo # 64 bit
except ImportError:
    import umundo64 as umundo # 32 bit

import time

node = umundo.Node();
pub = umundo.Publisher("chat");
sub = umundo.Subscriber("chat");

# TODO: actually implement the chat
time.sleep(10);
