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
using System.Collections.Generic;
using System.Linq;
using System.Text;

using org.umundo.core;
using Google.ProtocolBuffers;

namespace umundo.s11n
{
    public class TypedPublisher : Publisher
    {
        TypedPublisher(String channel) : base(channel) { 
        }

        public Message prepareMessage(String type, IMessageLite o)
        {
            Message msg = new Message();
            byte[] buffer = o.ToByteArray();
            msg.setData(buffer, (uint)buffer.Length);
            msg.putMeta("um.s11n.type", type);	
            return msg;
        }

        public void sendObject(String type, IMessageLite o)
        {
            send(prepareMessage(type, o));
        }

    }
}
