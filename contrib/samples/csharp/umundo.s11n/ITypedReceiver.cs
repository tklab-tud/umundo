using System;
using org.umundo.core;

namespace org.umundo.s11n
{
    public interface ITypedReceiver
    {
        /// <summary>
        /// The implementation should cast the received object to a more specific type.
        /// This can be done by inspecting the runtime type, or based on information in the
        /// original msg object 
        /// </summary>
        /// <param name="o">the deserialized object contained in the message. Needs to be cast to its actual type to do something.</param>
        /// <param name="msg">the original message, mainly to access meta information</param>
	    void receiveObject(Object o, Message msg);
    }
}
