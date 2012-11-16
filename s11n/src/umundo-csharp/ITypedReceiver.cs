/**
 *  @file
 *  @brief      Reciever for serialized objects
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


using org.umundo.core;

namespace org.umundo.s11n
{
    /// <summary>
    /// Objects implementing this interface will be notified, once an object has been received by a typed subscriber.
    /// </summary>
    public interface ITypedReceiver
    {
        /// <summary>
        /// The implementation should cast the received object to a more specific type.
        /// This can be done by inspecting the runtime type, or based on information in the
        /// original msg object 
        /// </summary>
        /// <param name="o">the deserialized object contained in the message. Needs to be cast to its actual type to do something.</param>
        /// <param name="msg">the original message, mainly to access meta information</param>
	    void ReceiveObject(Object o, Message msg);
    }
}
