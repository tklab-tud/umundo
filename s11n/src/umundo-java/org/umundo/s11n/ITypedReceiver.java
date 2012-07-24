/**
 *  Copyright (C) 2012  Daniel Schreiber
 *
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
 */

package org.umundo.s11n;

import org.umundo.core.Message;

public interface ITypedReceiver {

	/** The implementation should cast the received object to a more specific type. This can be done by inspecting the runtime type, or based on information in the original msg object 
	 * 
	 * @param object the deserialized object contained in the message. Needs to be cast to its actual type to do something.
	 * @param msg the original message, mainly to access meta information
	 */
	public void receiveObject(Object object, Message msg);
}
