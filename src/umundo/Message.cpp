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

#include "umundo/Message.h"
#include "umundo/config.h"

#ifdef BUILD_WITH_COMPRESSION_MINIZ
#include "miniz.h"
#endif
#if defined(BUILD_WITH_COMPRESSION_FASTLZ)
#include "fastlz.h"
#endif
#if defined(BUILD_WITH_COMPRESSION_LZ4)
#include "lz4.h"
#endif

#if 0
#include <boost/detail/endian.hpp>
#include <boost/lexical_cast.hpp>
#endif

namespace umundo {
char* Message::write(char* to, uint64_t value) {
	for(int i = 0; i < 8; i++) to[i] = value >> (8-1-i)*8;
	return to + 8;
}
char* Message::write(char* to, uint32_t value) {
	for(int i = 0; i < 4; i++) to[i] = value >> (4-1-i)*8;
	return to + 4;
}
char* Message::write(char* to, uint16_t value) {
	for(int i = 0; i < 2; i++) to[i] = value >> (2-1-i)*8;
	return to + 2;
}
char* Message::write(char* to, uint8_t value) {
	to[0] = value;
	return to + 1;
}
char* Message::write(char* to, int64_t value) {
	for(int i = 0; i < 8; i++) to[i] = value >> (8-1-i)*8;
	return to + 8;
}
char* Message::write(char* to, int32_t value) {
	for(int i = 0; i < 4; i++) to[i] = value >> (4-1-i)*8;
	return to + 4;
}
char* Message::write(char* to, int16_t value) {
	for(int i = 0; i < 2; i++) to[i] = value >> (2-1-i)*8;
	return to + 2;
}
char* Message::write(char* to, int8_t value) {
	to[0] = value;
	return to + 1;
}
char* Message::write(char* to, float value) {
//	to = (char*)&value;
	memcpy(to, &value, sizeof(float));
	return to + sizeof(float);
}
char* Message::write(char* to, double value) {
//	to = (char*)&value;
	memcpy(to, &value, sizeof(double));
	return to + sizeof(double);
}

const char* Message::read(const char* from, uint64_t* value) {
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
    
const char* Message::read(const char* from, uint32_t* value) {
	*value = ((from[0] << 24) & 0xFF000000U)
	         | ((from[1] << 16) & 0x00FF0000U)
	         | ((from[2] <<  8) & 0x0000FF00U)
	         |  (from[3]        & 0x000000FFU);
	return from + 4;
}

const char* Message::read(const char* from, uint16_t* value) {
	*value = ((from[0] <<  8) & 0xFF00U)
	         |  (from[1]        & 0x00FFU);
	return from + 2;
}
const char* Message::read(const char* from, uint8_t* value) {
	*value	= from[0];
	return from + 1;
}

    
char* Message::writeCompact(char* to, uint64_t value, size_t remaining) {
    // same approach as with websocket data frames
    if (value < 254 && remaining >= 1) {
        to[0] = value;
        return to + 1;
    } else if (value < (1 << 16) && remaining >= 3) {
        to[0] = 254;
        to++;
        return write(to, (uint16_t)value);
    } else if (remaining >= 9) {
        to[0] = 255;
        to++;
        return write(to, (uint64_t)value);
    } else {
        return 0;
    }
}
    
const char* Message::readCompact(const char* from, uint64_t* value, size_t remaining) {
    uint8_t type = 0;
    from = read(from, &type);
    if (type < 254 && remaining >= 1) {
        *value = type;
        return from;
    } else if (type == 254 && remaining >= 3) {
        return read(from, (uint16_t*)value);
    } else if (remaining >= 9) {
        return read(from, (uint64_t*)value);
    } else {
        return 0;
    }
}

const char* Message::read(const char* from, int64_t* value) {
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

const char* Message::read(const char* from, int32_t* value) {
	*value = ((from[0] << 24) & 0xFF000000U)
	         | ((from[1] << 16) & 0x00FF0000U)
	         | ((from[2] <<  8) & 0x0000FF00U)
	         |  (from[3]        & 0x000000FFU);
	return from + 4;
}

const char* Message::read(const char* from, int16_t* value) {
	*value = ((from[0] <<  8) & 0xFF00U)
	         |  (from[1]        & 0x00FFU);
	return from + 2;
}

const char* Message::read(const char* from, int8_t* value) {
	*value	= from[0];
	return from + 1;
}

const char* Message::read(const char* from, float* value) {
	*value = *(float*)from;
//	memcpy(value, from, sizeof(float));
	return from + sizeof(float);
}
const char* Message::read(const char* from, double* value) {
	*value = *(double*)from;
//	memcpy(value, from, sizeof(double));
	return from + sizeof(double);
}

char* Message::write(char* to, const std::string& value, bool terminate) {
	size_t writeSize = strnlen(value.c_str(), value.size()); // zero byte might occur before end
	memcpy(to, value.data(), writeSize);
	to += writeSize;
	if (terminate) {
		to[0] = '\0';
		to += 1;
	}
	return to;
}

const char* Message::read(const char* from, std::string& value, size_t maxLength) {
	size_t readSize = strnlen(from, maxLength);
	value = std::string(from, readSize);
	if (readSize == maxLength)
		return from + readSize;
	return from + readSize + 1; // we consumed \0
}

char* Message::writeHeaders(char* to, size_t size) {
    std::map<std::string, std::string>::const_iterator metaIter;
    for (metaIter = _meta.begin(); metaIter != _meta.end(); metaIter++) {
        if (metaIter->first.size() == 0) // we do not accept empty keys!
            continue;
        if (size > metaIter->first.size()) {
            to = write(to, metaIter->first);
            size -= metaIter->first.size() + 1;
        } else {
            return 0;
        }
        
        if (size > metaIter->second.size()) {
            to = write(to, metaIter->second);
            size -= metaIter->second.size() + 1;
        } else {
            return 0;
        }
    }
    return to;
}

const char* Message::readHeaders(const char* from, size_t size) {
    const char* readPtr = from;
    while(readPtr - from < size) {
        std::string key;
        std::string value;
        readPtr = Message::read(readPtr, key, size - (readPtr - from));
        readPtr = Message::read(readPtr, value, size - (readPtr - from));
        putMeta(key, value);
    }
    return readPtr;
}

struct comprCtxLZ4 {
    LZ4_stream_t* comprHeadStream;
    LZ4_streamDecode_t* deComprHeadStream;
    LZ4_stream_t* comprDataStream;
    LZ4_streamDecode_t* deComprDataStream;
    char* dataDict;
    size_t dataDictSize;
    char* headDict;
    size_t headDictSize;
};

    
size_t Message::compress(const std::string& name, void* ctx, char* data, size_t size, Compression type, int level) {
    // TODO: fall back to standard compression for NULL ctx
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)ctx;
    size_t compressedSize = 0;
    size_t dataSize       = (type == HEADER ? getHeaderDataSize() : _size);

    char* dict = NULL;
    size_t* dictSize = NULL;
    LZ4_stream_t* stream = NULL;
    
    char* buffer = _data.get();
    if (type == HEADER) {
        buffer = (char*)malloc(dataSize);
        writeHeaders(buffer, dataSize);
    }
    if (c != NULL) {
        // compression with a context
        stream         = (type == HEADER ? c->comprHeadStream : c->comprDataStream);
        dict           = (type == HEADER ? c->headDict : c->dataDict);
        dictSize       = (type == HEADER ? &c->headDictSize : &c->dataDictSize);
        compressedSize = LZ4_compress_fast_continue(stream, buffer, data, dataSize, size, level);
        if (compressedSize <= 0)
            return 0;
        
        *dictSize = dataSize < (1 << 16) ? dataSize : (1 << 16);
        LZ4_saveDict(stream, dict, *dictSize);

    } else {
        // compression without a context
        compressedSize = LZ4_compress_fast(buffer, data, dataSize, size, level);
        if (compressedSize <= 0)
            return 0;
    }
    
    if (type == HEADER) {
        free(buffer);
    }
    
    return compressedSize;

}

#define DECPOMPRESS_HEAD_SIZE_FACTOR 2
#define DECPOMPRESS_DATA_SIZE_FACTOR 2

size_t Message::uncompress(const std::string& name,
                           void* ctx,
                           const char* data,
                           size_t size,
                           Compression type,
                           size_t origSize) {

    if (size == 0)
        return 0;
    
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)ctx;
    int decBytes = 0;
    
    LZ4_streamDecode_t* stream = NULL;
    char* dict                 = NULL;
    size_t* dictSize           = NULL;
    size_t scaleFactor = (type == HEADER ? DECPOMPRESS_HEAD_SIZE_FACTOR : DECPOMPRESS_DATA_SIZE_FACTOR);

    int decBufferSize = (origSize > 0 ? origSize : size * scaleFactor);
    char* uncompressed = (char*)malloc(decBufferSize);

    if (c != NULL) {
        stream   = (type == HEADER ? c->deComprHeadStream : c->deComprDataStream);
        dict     = (type == HEADER ? c->headDict : c->dataDict);
        dictSize = (type == HEADER ? &c->headDictSize : &c->dataDictSize);

        // set the dictionary
        if (*dictSize > 0)
            LZ4_setStreamDecode(stream, dict, *dictSize);
    }

    while(true) {
        if (c != NULL) {
            // uncompress with given context
            decBytes = LZ4_decompress_safe_continue(stream, data, uncompressed, size, decBufferSize);
        } else {
            // uncompress without context
            decBytes = LZ4_decompress_safe(data, uncompressed, size, decBufferSize);
        }
        
        if (decBytes > 0) {
            // success
            if (c != NULL) {
                // save dictionary and break
                *dictSize = decBytes < (1 << 16) ? decBytes : (1 << 16);
                memcpy(dict, &uncompressed[decBytes - *dictSize], *dictSize);
            }
            break;
        }
        
        if (decBytes < 0 && decBytes * -1 >= size) {
            // we failed!
            free (uncompressed);
            return 0;
        }
        
        // remember old position and increase buffer
        decBufferSize += size * scaleFactor + (1 << 16);
        
        if (decBufferSize > (1 << 30) || decBufferSize < 0) {
            // decompressed data is getting too large
            free (uncompressed);
            return 0;
        }
        
        uncompressed = (char*)realloc(uncompressed, decBufferSize);
    }

    // safe decompressed byte array as headers or data
    if (type == HEADER) {
        _meta["um.compressRatio.head"] = toStr(100 * ((double)size / (double)decBytes));
        readHeaders(uncompressed, decBytes);
        free(uncompressed);
    } else {
        uncompressed = (char*)realloc(uncompressed, decBytes);
        _meta["um.compressRatio.payload"] = toStr(100 * ((double)size / (double)decBytes));
        _data = SharedPtr<char>(uncompressed);
        _size = decBytes;
    }
        
    return decBytes;
}

void* Message::createCompression() {
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)malloc(sizeof(comprCtxLZ4));
    
    c->comprDataStream = LZ4_createStream();
    c->comprHeadStream = LZ4_createStream();
    c->deComprDataStream = LZ4_createStreamDecode();
    c->deComprHeadStream = LZ4_createStreamDecode();
    
    c->dataDictSize = 0;
    c->dataDict = (char*)malloc((1 << 16));
    memset(c->dataDict, 0, (1 << 16));
    
    c->headDictSize = 0;
    c->headDict = (char*)malloc((1 << 16));
    memset(c->headDict, 0, (1 << 16));

    return c;
}

void Message::freeCompression(void* ctx) {
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)ctx;

    // make sure all lingering data is reset
    resetCompression(ctx);
    
    LZ4_freeStream(c->comprDataStream);
    LZ4_freeStream(c->comprHeadStream);
    LZ4_freeStreamDecode(c->deComprDataStream);
    LZ4_freeStreamDecode(c->deComprHeadStream);
    
    free(c->dataDict);
    free(c->headDict);
}

void Message::resetCompression(void* ctx) {
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)ctx;

    LZ4_resetStream(c->comprDataStream);
    LZ4_resetStream(c->comprHeadStream);
    memset(c->deComprDataStream, 0, sizeof(LZ4_streamDecode_t));
    memset(c->deComprHeadStream, 0, sizeof(LZ4_streamDecode_t));

    memset(c->dataDict, 0, c->dataDictSize);
    memset(c->headDict, 0, c->headDictSize);
}

size_t Message::getCompressBounds(const std::string& name, void* ctx, Compression type) {
    struct comprCtxLZ4* c = (struct comprCtxLZ4*)ctx;
    (void)c;
    
    switch (type) {
        case HEADER:
            return LZ4_compressBound(getHeaderDataSize());
        case PAYLOAD:
            return LZ4_compressBound(_size);
        default:
            return 0;
    }
}

size_t Message::getHeaderDataSize() {
    size_t headerDataSize = 0;
    std::map<std::string, std::string>::const_iterator metaIter;
    for (metaIter = _meta.begin(); metaIter != _meta.end(); metaIter++) {
        if (metaIter->first.size() == 0) // we do not accept empty keys!
            continue;
        headerDataSize += metaIter->first.size() + 1;
        headerDataSize += metaIter->second.size() + 1;
    }
    return headerDataSize;
}

}
