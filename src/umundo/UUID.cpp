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

#include "umundo/UUID.h"

// silence the warning

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

#include <iostream>

namespace umundo {

boost::uuids::random_generator randomGen;
const std::string UUID::getUUID() {
	return boost::lexical_cast<std::string>(randomGen());
}

// see
// http://codereview.stackexchange.com/questions/78535/converting-array-of-bytes-to-the-hex-string-representation
// http://codereview.stackexchange.com/questions/22757/char-hex-string-to-byte-array?rq=1
    
static const char binToHexMap[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
static const char hexToBinMap[0x80] = //This is the ASCII table in number value form
    {   //0     1     2     3     4     5     6    7      8     9     A     B     C     D     E     F
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //2
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //3
        0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //4
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //5
        0x00, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //6
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  //7
    };

std::string UUID::hexToBin(const std::string& hex) {
    std::string binUUID;
    binUUID.resize(16);
    writeHexToBin(&binUUID[0], hex);
    return binUUID;
}

char* UUID::writeHexToBin(char* to, const std::string& hex) {
    if (hex.length() != 36)
        return to;
    
    
    for (size_t writePos = 0, readPos = 0; readPos < 36 && writePos < 16; readPos += 2, writePos++) {
        while (hex[readPos] == '-')
            readPos++;
        to[writePos]     =  hexToBinMap[(uint8_t)hex[readPos]] << 4;
        to[writePos]     += hexToBinMap[(uint8_t)hex[readPos + 1]];
//        const char* data = binUUID.data();
//        data = NULL;
    }
    return to + 16;
}

const char* UUID::readBinToHex(const char* from, std::string& hex) {
    if (hex.size() < 36)
        hex.resize(36);
 
    for (size_t pos = 0, i = 0; i < 16 && pos < 36; i++, pos += 2) {
        
        if (pos == 8 || pos == 13 || pos == 18 || pos == 23) {
            hex[pos] = '-';
            pos++;
        }
        
        hex[pos]     = (binToHexMap[(from[i] & 0xf0) >> 4]);
        hex[pos+1]   = (binToHexMap[(from[i] & 0x0f)]);
        
        
    }
    return from + 16;

}

std::string UUID::binToHex(const std::string& bin) {
    std::string hexUUID;
    hexUUID.resize(36);

    readBinToHex(&bin[0], hexUUID);
    
    return hexUUID;
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
