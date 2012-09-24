/**
 *  @file
 *  @brief      Subscriber for serialized objects
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @copyright  Simplified BSD
 *
 *  @cond
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 *  @endcond
 */

using System;
using System.IO;
using org.umundo.core;
using ProtoBuf.Meta;

namespace org.umundo.s11n
{
    public class TypedSubscriber : Subscriber
    {
        class RawReceiver : Receiver {

            public RawReceiver(ITypedReceiver rcv) {
                TypedReceiver = rcv;
            }

            internal ITypedReceiver TypedReceiver { get; private set; }

            public override void receive(Message msg) {
                String typename = msg.getMeta("um.s11n.type");
                byte[] data = msg.getData();
                Type type = Type.GetType(typename);
                Stream source = new MemoryStream(data);
                Object o = RuntimeTypeModel.Default.Deserialize(source, null, type);
                TypedReceiver.receiveObject(o, msg);
            }
        }

        public TypedSubscriber(String channel, ITypedReceiver receiver)
            : base(channel)
        {
            Receiver = new RawReceiver(receiver);
            setReceiver(Receiver);
        }

        private RawReceiver Receiver { get; set; }
    }
}
