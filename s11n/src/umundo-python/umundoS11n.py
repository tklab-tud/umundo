import sys

try:
    import umundo64 as umundo_proto
except:
    import umundo as umundo_proto


class TypedPublisher(umundo_proto.Publisher):
    def prepareMessage(self, protoType, messageLiteObject):
        msg = umundo_proto.Message()
        buffer = messageLiteObject.SerializeToString()
        msg.setData(buffer)
        msg.putMeta("um.s11n.type", protoType)
        return msg

    def sendObject(self, protoType, messageLiteObject):
		self.send(self.prepareMessage(protoType, messageLiteObject))



class TypedSubscriber(umundo_proto.Subscriber):

    def __init__(self, channel, receiver):
        umundo_proto.Subscriber.__init__(self, channel)
        self.channel = channel
        self.receiver = receiver
        decoratedReceiver = TypedSubscriber.DeserializingReceiverDecorator(receiver)

        self.setReceiver(decoratedReceiver)

    class DeserializingReceiverDecorator(umundo_proto.Receiver):

        def __init__(self, receiver):
            self.receiver = receiver

        def receive(self, msg):
            print(msg)
            if (not "um.s11n.type" in msg.getMeta()):
                return self.receiver.receiveObject(msg)
            protoType = msg.getMeta("um.s11n.type");
            data = msg.getData();

            print("type: "+protoType)
            print("data: "+data)
            return None



class TypedReceiver:
    def receiveObject(obj, msg):
        pass


class TypedGreeter:
    def welcome(self, atPub, nodeId, subId):
        pass

    def farewell(self, fromPub, nodeId, subId):
        pass

    def prepareMessage(self, type, msgLite):
        pass

