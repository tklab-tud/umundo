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
using System.Collections.Generic;
using org.umundo.core;
using ProtoBuf.Meta;

namespace org.umundo.s11n
{
    public class TypedSubscriber : Subscriber
    {
        class RawReceiver : Receiver {

            private Dictionary<string, Type> types;

            public RawReceiver(ITypedReceiver rcv) {
                types = new Dictionary<string, Type>();
                TypedReceiver = rcv;
            }

            internal ITypedReceiver TypedReceiver { get; private set; }

            public override void receive(Message msg) {
                String typename = msg.getMeta("um.s11n.type");
                string str = msg.getData();
                char[] buffer = new char[str.Length];
                str.CopyTo(0, buffer, 0, buffer.Length);
                byte[] data = Array.ConvertAll(buffer, x => (byte)x);
                Type type;
                if (types.ContainsKey(typename))
                {
                    type = types[typename];
                }
                else
                {
                    type = Type.GetType(typename);
                }
                Stream source = new MemoryStream(data);
                Object o = RuntimeTypeModel.Default.Deserialize(source, null, type);
                TypedReceiver.receiveObject(o, msg);
            }

            public void RegisterType(string typename, Type type)
            {
                types.Add(typename, type);
            }
        }

        public TypedSubscriber(String channel, ITypedReceiver receiver)
            : base(channel)
        {
            Receiver = new RawReceiver(receiver);
            setReceiver(Receiver);
        }

        private RawReceiver Receiver { get; set; }

        public void RegisterType(string typename, Type type)
        {
            Receiver.RegisterType(typename, type);
        }
    }
}
