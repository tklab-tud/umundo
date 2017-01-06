/**
 *  @file
 *  @brief      Generates 36 byte UUID strings.
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

#ifndef UUID_H_ASB7D2U4
#define UUID_H_ASB7D2U4

#include "umundo/Common.h"

namespace umundo {

/**
 * UUID Generator for 36 byte UUIDs
 */
class UMUNDO_API UUID {
public:
	static const std::string getUUID();
	static bool isUUID(const std::string& uuid);

    static std::string hexToBin(const std::string& hex);
    static char* writeHexToBin(char* to, const std::string& hex);
    static std::string binToHex(const std::string& bin);
    static const char* readBinToHex(const char* from, std::string& hex);

private:
	UUID() {}
};

}

#endif /* end of include guard: UUID_H_ASB7D2U4 */
