/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#ifndef RESULTSET_H_D2O6OBDA
#define RESULTSET_H_D2O6OBDA

#include "umundo/common/Common.h"

namespace umundo {

template<class T>

/**
 * Templated interface to be notified about addition, removal or changes of objects.
 */
class DLLEXPORT ResultSet {
public:
	virtual void added(shared_ptr<T>) = 0;
	virtual void removed(shared_ptr<T>) = 0;
	virtual void changed(shared_ptr<T>) = 0;
};
}

#endif /* end of include guard: RESULTSET_H_D2O6OBDA */
