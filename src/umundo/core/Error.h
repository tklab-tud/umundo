/**
 *  @file
 *  @brief      Error representation.
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


#ifndef ERROR_H_BX84MNOA
#define ERROR_H_BX84MNOA

namespace umundo {

class Error {
	operator bool() {
		return errorCode != 0;
	}
	Error() : errorCode(0) {}
	std::string file;
	std::string line;
	std::string module;
	std::string message;
	uint16_t errorCode;
};

}

#endif /* end of include guard: ERROR_H_BX84MNOA */
