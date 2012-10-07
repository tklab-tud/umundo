/**
 *  Copyright (C) 2012  Stefan Radomski
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

public interface ITypedGreeter {

	public void welcome(TypedPublisher atPub, String nodeId, String subId);
	public void farewell(TypedPublisher fromPub, String nodeId, String subId);

}
