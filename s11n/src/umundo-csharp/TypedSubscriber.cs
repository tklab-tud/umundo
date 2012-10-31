/**
 *  @file
 *  @brief      Subscriber for serialized objects
 *  @author     2012 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *  @author     2012 Dirk Schnelle-Walka
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
using System.Collections.Generic;
using System.IO;
using org.umundo.core;
using ProtoBuf.Meta;

namespace org.umundo.s11n
{
    /// <summary>
    /// A typed subscriber is able to receive objects that have been sent over umundo.
    /// </summary>
    public class TypedSubscriber : Subscriber
    {
        class RawReceiver : Receiver
        {
            private Dictionary<string, Type> types;

            /// <summary>
            /// Constructs a new publisher for the given channel name.
            /// </summary>
            /// <param name="channel">name of the channel</param>
            public RawReceiver(ITypedReceiver rcv) {
                types = new Dictionary<string, Type>(); 
                TypedReceiver = rcv;
            }

            internal ITypedReceiver TypedReceiver { get; private set; }

            private byte[] GetBytes(string str)
            {
                byte[] bytes = new byte[str.Length * sizeof(char)];
                System.Buffer.BlockCopy(str.ToCharArray(), 0, bytes, 0, bytes.Length);
                return bytes;
            }

            public override void receive(Message msg) {
                String typename = msg.getMeta("um.s11n.type");
                
                byte[] data = msg.getData();
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

            /// <summary>
            /// Registers the given type name so that it can be instantiated once it is received.
            /// </summary>
            /// <param name="typename">fully qualified type name</param>
            /// <param name="type">associated type</param>
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

        /// <summary>
        /// Registers the given type name so that it can be instantiated once it is received.
        /// </summary>
        /// <param name="typename">fully qualified type name</param>
        /// <param name="type">associated type</param>
        public void RegisterType(string typename, Type type)
        {
            Receiver.RegisterType(typename, type);
        }

        /// <summary>
        /// Registers the given type name so that it can be instantiated once it is received.
        /// </summary>
        /// <param name="o">the object to register</param>
        public void RegisterType(object o)
        {
            Type type = o.GetType();
            Receiver.RegisterType(type.Name, type);
        }

        /// <summary>
        /// Registers the given type name so that it can be instantiated once it is received.
        /// </summary>
        /// <param name="type">the type to register</param>
        public void RegisterType(Type type)
        {
            Receiver.RegisterType(type.Name, type);
        }
    }
}
