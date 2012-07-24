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

#ifndef UUID_H_ASB7D2U4
#define UUID_H_ASB7D2U4

#include "umundo/common/Common.h"
#include <boost/uuid/uuid_generators.hpp>

namespace umundo {

class DLLEXPORT UUID {
public:
	static const string getUUID();

private:
	static boost::uuids::random_generator randomGen;
};

}

#endif /* end of include guard: UUID_H_ASB7D2U4 */
