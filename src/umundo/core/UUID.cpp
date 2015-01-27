/**
 *  @file
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

#include "umundo/core/UUID.h"

// silence the warning

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

namespace umundo {

boost::uuids::random_generator UUID::randomGen;
const std::string UUID::getUUID() {
	return boost::lexical_cast<std::string>(randomGen());
}

bool UUID::isUUID(const std::string& uuid) {
	if (uuid.size() != 36)
		return false;

	if (uuid[8] != '-' || uuid[13] != '-' || uuid[18] != '-' || uuid[23] != '-')
		return false;

	for (int i = 0; i < 36; i++) {
		if (i == 8 || i == 13 || i == 18 || i ==23)
			continue;

		char c = uuid[i];
		if (c == 'a' ||
		        c == 'b' ||
		        c == 'c' ||
		        c == 'd' ||
		        c == 'e' ||
		        c == 'f' ||
		        c == '0' ||
		        c == '1' ||
		        c == '2' ||
		        c == '3' ||
		        c == '4' ||
		        c == '5' ||
		        c == '6' ||
		        c == '7' ||
		        c == '8' ||
		        c == '9') {
			continue;
		} else {
			return false;
		}
	}
	return true;
}

}
