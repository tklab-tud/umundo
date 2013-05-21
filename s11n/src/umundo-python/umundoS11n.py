import sys, traceback, zlib

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
        print "-> send %s: size=%s, crc32=%s, data='%s'" % (protoType, msg.size(), zlib.crc32(buffer), buffer)
        return msg

    def sendObject(self, name, messageLiteObject):
        self.send(self.prepareMessage(name, messageLiteObject))


class TypedSubscriber(umundo_proto.Subscriber):
    def __init__(self, channel, receiver):
        super(TypedSubscriber, self).__init__(channel)
        self.deserializerMethods = dict()
        self.channel = channel
        self.receiver = receiver
        self.decoratedReceiver = DeserializingReceiverDecorator(self, receiver)
        self.setReceiver(self.decoratedReceiver)

    def registerType(self, name, clazz):
        self.deserializerMethods[name] = clazz


class DeserializingReceiverDecorator(umundo_proto.Receiver):
    def __init__(self, typedSubscriber, receiver):
        super(DeserializingReceiverDecorator, self).__init__()
        self.receiver = receiver
        self.typedSubscriber = typedSubscriber

    def receive(self, msg):
        try:
            metaType = msg.getMeta("um.s11n.type")
            if (metaType == None):
                return
            elif (not metaType in self.typedSubscriber.deserializerMethods):
                return
            else:
                data = msg.data();
                print "-> receive %s: size=%d, crc32=%s, data='%s'" % (metaType, msg.size(), zlib.crc32(data), data)
                instance = self.typedSubscriber.deserializerMethods[metaType]()
                instance.ParseFromString(data)
                return self.receiver.receiveObject(instance, msg)
        except BaseException as e:
            print("failed received")
            traceback.print_exc(file=sys.stdout)


class TypedReceiver(umundo_proto.Receiver):
    def __init__(self, *args):
        super(TypedReceiver,self).__init__(*args)


class TypedGreeter(umundo_proto.Greeter):
    def __init__(self, *args):
        super(TypedGreeter,self).__init__(*args)
