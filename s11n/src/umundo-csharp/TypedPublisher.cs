/**
 *  @file
 *  @brief      Publisher for serialized objects
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
        /// <summary>
        /// Constructs a new publisher for the given channel name.
        /// </summary>
        /// <param name="channel">name of the channel</param>
        public TypedPublisher(String channel) : base(channel) { 
        }

        private Message prepareMessage(String type, ISerializable o)
        {
            Message msg = new Message();
            using (MemoryStream stream = new MemoryStream())
            {
                Serializer.Serialize(stream, o);
                stream.Position = 0;
                StreamReader reader = new StreamReader(stream);
                string buffer = reader.ReadToEnd();
                msg.setData(buffer, (uint)buffer.Length);
            }
            msg.putMeta("um.s11n.type", type);	
            return msg;
        }

        private Message prepareMessage(String type, IExtensible o)
        {
            Message msg = new Message();
            using (MemoryStream stream = new MemoryStream())
            {
                Serializer.Serialize(stream, o);
                stream.Position = 0;
                StreamReader reader = new StreamReader(stream);
                string buffer = reader.ReadToEnd();
                msg.setData(buffer, (uint)buffer.Length);
            }
            msg.putMeta("um.s11n.type", type);
            return msg;
        }

        /// <summary>
        /// Sends the serializable object. This method is being called from umundo.
        /// </summary>
        /// <param name="type">type of the object to send</param>
        /// <param name="o">the object to send</param>
        public void sendObject(String type, ISerializable o)
        {
            Message message = prepareMessage(type, o);
            send(message);
        }

        /// <summary>
        /// Sends the protobuf serializable object. This method is being called from umundo.
        /// </summary>
        /// <param name="type">type of the object to send</param>
        /// <param name="o">the object to send</param>
        public void sendObject(String type, IExtensible o)
        {
            Message message = prepareMessage(type, o);
            send(message);
        }
    }
}
