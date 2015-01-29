/**
 *  @file
 *  @author     2015 Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
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

#include "umundo/core/Message.h"
#include "umundo/config.h"
#include "miniz.h"

#if 0
#include <boost/detail/endian.hpp>
#include <boost/lexical_cast.hpp>
#endif

namespace umundo {
char* Message::write(uint64_t value, char* to) {
	for(int i = 0; i < 8; i++) to[i] = value >> (8-1-i)*8;
	return to + 8;
}
char* Message::write(uint32_t value, char* to) {
	for(int i = 0; i < 4; i++) to[i] = value >> (4-1-i)*8;
	return to + 4;
}
char* Message::write(uint16_t value, char* to) {
	for(int i = 0; i < 2; i++) to[i] = value >> (2-1-i)*8;
	return to + 2;
}
char* Message::write(uint8_t value, char* to) {
	to[0] = value;
	return to + 1;
}
char* Message::write(int64_t value, char* to) {
	for(int i = 0; i < 8; i++) to[i] = value >> (8-1-i)*8;
	return to + 8;
}
char* Message::write(int32_t value, char* to) {
	for(int i = 0; i < 4; i++) to[i] = value >> (4-1-i)*8;
	return to + 4;
}
char* Message::write(int16_t value, char* to) {
	for(int i = 0; i < 2; i++) to[i] = value >> (2-1-i)*8;
	return to + 2;
}
char* Message::write(int8_t value, char* to) {
	to[0] = value;
	return to + 1;
}
char* Message::write(float value, char* to) {
//	to = (char*)&value;
	memcpy(to, &value, sizeof(float));
	return to + sizeof(float);
}
char* Message::write(double value, char* to) {
//	to = (char*)&value;
	memcpy(to, &value, sizeof(double));
	return to + sizeof(double);
}

const char* Message::read(uint64_t* value, const char* from) {
	*value = (((uint64_t)from[0] << 56) & 0xFF00000000000000ULL)
	       | (((uint64_t)from[1] << 48) & 0x00FF000000000000ULL)
	       | (((uint64_t)from[2] << 40) & 0x0000FF0000000000ULL)
	       | (((uint64_t)from[3] << 32) & 0x000000FF00000000ULL)
	       | ((from[4] << 24) & 0x00000000FF000000U)
	       | ((from[5] << 16) & 0x0000000000FF0000U)
	       | ((from[6] <<  8) & 0x000000000000FF00U)
	       |  (from[7]        & 0x00000000000000FFU);
	return from + 8;
}
const char* Message::read(uint32_t* value, const char* from) {
	*value = ((from[0] << 24) & 0xFF000000U)
	       | ((from[1] << 16) & 0x00FF0000U)
	       | ((from[2] <<  8) & 0x0000FF00U)
	       |  (from[3]        & 0x000000FFU);
	return from + 4;
}
	
const char* Message::read(uint16_t* value, const char* from) {
	*value = ((from[0] <<  8) & 0xFF00U)
	       |  (from[1]        & 0x00FFU);
	return from + 2;
}
const char* Message::read(uint8_t* value, const char* from) {
	*value	= from[0];
	return from + 1;
}

const char* Message::read(int64_t* value, const char* from) {
	*value = (((int64_t)from[0] << 56) & 0xFF00000000000000ULL)
	       | (((int64_t)from[1] << 48) & 0x00FF000000000000ULL)
	       | (((int64_t)from[2] << 40) & 0x0000FF0000000000ULL)
	       | (((int64_t)from[3] << 32) & 0x000000FF00000000ULL)
	       | ((from[4] << 24) & 0x00000000FF000000U)
	       | ((from[5] << 16) & 0x0000000000FF0000U)
	       | ((from[6] <<  8) & 0x000000000000FF00U)
	       |  (from[7]        & 0x00000000000000FFU);
	return from + 8;
}

const char* Message::read(int32_t* value, const char* from) {
	*value = ((from[0] << 24) & 0xFF000000U)
	       | ((from[1] << 16) & 0x00FF0000U)
	       | ((from[2] <<  8) & 0x0000FF00U)
	       |  (from[3]        & 0x000000FFU);
	return from + 4;
}

const char* Message::read(int16_t* value, const char* from) {
	*value = ((from[0] <<  8) & 0xFF00U)
	       |  (from[1]        & 0x00FFU);
	return from + 2;
}

const char* Message::read(int8_t* value, const char* from) {
	*value	= from[0];
	return from + 1;
}

const char* Message::read(float* value, const char* from) {
	*value = *(float*)from;
//	memcpy(value, from, sizeof(float));
	return from + sizeof(float);
}
const char* Message::read(double* value, const char* from) {
	*value = *(double*)from;
//	memcpy(value, from, sizeof(double));
	return from + sizeof(double);
}

void Message::compress() {
	if (isCompressed())
		return;
	
	mz_ulong compressedSize = mz_compressBound(_size);
	int cmp_status;
	uint8_t *pCmp;

	pCmp = (mz_uint8 *)malloc((size_t)compressedSize);

//	mz_ulong tmpSize = _size;
	cmp_status = mz_compress(pCmp, &compressedSize, (const unsigned char *)_data.get(), _size);
	if (cmp_status != Z_OK) {
		// error
		free(pCmp);
	}
	
	_data = SharedPtr<char>((char*)pCmp);
	_meta["um.compressed"] = toStr(_size);
	_size = compressedSize;
}

void Message::uncompress() {
	if (!isCompressed())
		return;
	
	int cmp_status;
	mz_ulong actualSize = strTo<size_t>(_meta["um.compressed"]);
	uint8_t *pUncmp;

	pUncmp = (mz_uint8 *)malloc((size_t)actualSize);
	cmp_status = mz_uncompress(pUncmp, &actualSize, (const unsigned char *)_data.get(), _size);

	if (cmp_status != Z_OK) {
		// error
		free(pUncmp);
	}

	_size = actualSize;
	_data = SharedPtr<char>((char*)pUncmp);
	_meta.erase("um.compressed");
}

}