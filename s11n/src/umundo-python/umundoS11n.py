import sys, traceback

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

    def sendObject(self, messageLiteObject):
        protoType = str(type(messageLiteObject).__name__)
        print("send type:%s"%protoType)
        self.send(self.prepareMessage(protoType, messageLiteObject))


class TypedSubscriber(umundo_proto.Subscriber):

    def __init__(self, channel, receiver):
        super(TypedSubscriber, self).__init__(channel)
        self.deserializerMethods = dict()
        self.channel = channel
        self.receiver = receiver
        self.decoratedReceiver = DeserializingReceiverDecorator(self, receiver)
        self.setReceiver(self.decoratedReceiver)

    def registerType(self, name, clazz):
        print(type(clazz))
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
                print("unkown msg")
                return
            elif (not metaType in self.typedSubscriber.deserializerMethods):
                print("unkown type %s"%metaType)
                for k in self.typedSubscriber.deserializerMethods:
                    print(k)
                    print(self.typedSubscriber.deserializerMethods[k])
            else:
                data = msg.data();
                #print(type(data))
                savedClazz = self.typedSubscriber.deserializerMethods[metaType]
                #print(type(savedClazz))
                instance =savedClazz()
                #print(type(instance))
                #print(dir(savedClazz))
                savedClazz.ParseFromString(instance, data)
                #print(instance)
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
