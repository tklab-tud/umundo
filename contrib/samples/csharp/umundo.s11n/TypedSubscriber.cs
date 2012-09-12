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
using System.Collections.Generic;
using System.Linq;
using System.Text;

using org.umundo.core;
using Google.ProtocolBuffers;

namespace umundo.s11n
{
    public class TypedReceiver { 
    }

    public class TypedSubscriber : Subscriber
    {
        class RawReceiver : Receiver {
            TypedSubscriber _sub;

            public RawReceiver(TypedSubscriber sub) {
                _sub = sub;
            }
            public virtual void receive(Message msg) { 
                
            }
        }

        RawReceiver _rawReceiver;
        TypedReceiver _typedReceiver;

        TypedSubscriber(String channel, TypedReceiver receiver)
            : base(channel)
        {
            _typedReceiver = receiver;
            _rawReceiver = new RawReceiver(this);
            setReceiver(_rawReceiver);
        }

    }
}
