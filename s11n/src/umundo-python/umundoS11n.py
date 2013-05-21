import sys

try:
    import umundo64 as umundo_proto
except:
    try:
        import umundo as umundo_proto
    except:
        try:
            import umundo64_d as umundo_proto
        except:
            import umundo_d as umundo_proto


class TypedPublisher(umundo_proto.Publisher):

    def __init__(self, *args):
        super(TypedPublisher,self).__init__(*args)

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
        super(TypedSubscriber, self).__init__(channel)
        self.deserializerMethods = dict()
        self.channel = channel
        self.receiver = receiver
        self.decoratedReceiver = DeserializingReceiverDecorator(receiver)
        self.setReceiver(self.decoratedReceiver)

    def registerType(self, generatedMessageType):
        self.deserializerMethods[generatedMessageType.__class__] = generatedMessageType


class DeserializingReceiverDecorator(umundo_proto.Receiver):

    def __init__(self, receiver):
        super(DeserializingReceiverDecorator,self).__init__()
        self.receiver = receiver

    def receive(self, msg):
        print("msg received, TODO:impl")
        if (not "um.s11n.type" in msg.getMeta()):
            return self.receiver.receiveObject(msg)
        protoType = msg.getMeta("um.s11n.type");
        data = msg.getData();
        print("type: "+protoType)
        print("data: "+data)
        return None



class TypedReceiver(umundo_proto.Receiver):

    def __init__(self, *args):
        super(TypedReceiver,self).__init__(*args)


class TypedGreeter(umundo_proto.Greeter):

    def __init__(self, *args):
        super(TypedGreeter,self).__init__(*args)
