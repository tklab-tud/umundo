/**
 *  @file
 *  @brief      Publisher for serialized objects
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
using System.IO;
using System.Runtime.Serialization;
using org.umundo.core;
using ProtoBuf;

namespace org.umundo.s11n
{
    /// <summary>
    /// A typed publsiher is able to serialize objects and send the over umundo.
    /// </summary>   
    public class TypedPublisher : Publisher
    {
        private GreeterDecorator greeterDecorator;

        /// <summary>
        /// Constructs a new publisher for the given channel name.
        /// </summary>
        /// <param name="channel">name of the channel</param>        
        public TypedPublisher(String channel)
            : base(channel) { 
        }

        private byte[] Serialize(ISerializable serializable)
        {
            using (MemoryStream stream = new MemoryStream())
            {
                Serializer.Serialize(stream, serializable);
                return stream.ToArray();
            }
        }


        private byte[] Serialize(IExtensible extensible)
        {
            using (MemoryStream stream = new MemoryStream())
            {
                Serializer.Serialize(stream, extensible);
                return stream.ToArray();
            }
        }

        private Message PrepareMessage(String type, byte[] buffer)
        {
            Message msg = new Message();
            msg.setData(buffer);
            msg.putMeta("um.s11n.type", type);	
            return msg;
        }


        /// <summary>
        /// Sends the serializable object.
        /// </summary>
        /// <param name="type">type of the object to send</param>
        /// <param name="o">the object to send</param>
        public void SendObject(String type, ISerializable o)
        {
            byte[] buffer = Serialize(o);
            Message message = PrepareMessage(type, buffer);
            send(message);
        }

        /// <summary>
        /// Sends the serializable object and autmatically determine the name.
        /// </summary>
        /// <param name="o">the object to send</param>
        public void SendObject(ISerializable o)
        {
            byte[] buffer = Serialize(o);
            string type = o.GetType().Name;
            Message message = PrepareMessage(type, buffer);
            send(message);
        }

        /// <summary>
        /// Sends the protobuf serializable object.
        /// </summary>
        /// <param name="type">type of the object to send</param>
        /// <param name="o">the object to send</param>
        public void SendObject(String type, IExtensible o)
        {
            byte[] buffer = Serialize(o);
            Message message = PrepareMessage(type, buffer);
            send(message);
        }

        /// <summary>
        /// Sends the protobuf serializable object autmatically determine the name.
        /// </summary>
        /// <param name="type">type of the object to send</param>
        public void SendObject(IExtensible o)
        {
            byte[] buffer = Serialize(o);
            string type = o.GetType().Name;
            Message message = PrepareMessage(type, buffer);
            send(message);
        }

        public ITypedGreeter ITypedGreeter
        {
            get
            {
                if (greeterDecorator == null)
                {
                    return null;
                }
                return greeterDecorator.Greeter;
            }

            set
            {
                if (greeterDecorator == null)
                {
                    greeterDecorator = new GreeterDecorator();
                    greeterDecorator.TypedPublisher = this;
                    base.setGreeter(greeterDecorator);
                }
                greeterDecorator.Greeter = value;
            }
        }
    }
}
