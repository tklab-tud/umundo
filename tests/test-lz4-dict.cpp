// LZ4 streaming API example : ring buffer
// Based on sample code from Takayuki Matsuoka


/**************************************
 * Compiler Options
 **************************************/
#ifdef _MSC_VER    /* Visual Studio */
#  define _CRT_SECURE_NO_WARNINGS // for MSVC
#  define snprintf sprintf_s
#endif
#ifdef __GNUC__
#  pragma GCC diagnostic ignored "-Wmissing-braces"   /* GCC bug 53119 : doesn't accept { 0 } as initializer (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53119) */
#endif


/**************************************
 * Includes
 **************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lz4.h"
#include "umundo.h"
#include "umundo/util/crypto/MD5.h"

#include <iostream>
#include <sstream>
#include <iomanip>

enum {
    MESSAGE_MAX_BYTES   = 1024,
    RING_BUFFER_BYTES   = 1024 * 8 + MESSAGE_MAX_BYTES,
    DECODE_RING_BUFFER  = RING_BUFFER_BYTES + MESSAGE_MAX_BYTES   // Intentionally larger, to test unsynchronized ring buffers
};


size_t write_int32(FILE* fp, int32_t i) {
    return fwrite(&i, sizeof(i), 1, fp);
}

size_t write_bin(FILE* fp, const void* array, int arrayBytes) {
    return fwrite(array, 1, arrayBytes, fp);
}

size_t read_int32(FILE* fp, int32_t* i) {
    return fread(i, sizeof(*i), 1, fp);
}

size_t read_bin(FILE* fp, void* array, int arrayBytes) {
    return fread(array, 1, arrayBytes, fp);
}


void test_compress(FILE* outFp, FILE* inpFp)
{
    LZ4_stream_t lz4Stream_body = { 0 };
    LZ4_stream_t* lz4Stream = &lz4Stream_body;
    
    size_t dictSize = MESSAGE_MAX_BYTES;
    char* dict = (char*)malloc(dictSize);
    
    static char inpBuf[MESSAGE_MAX_BYTES];
    int inpOffset = 0;
    
    for(;;) {
        // Read random length ([1,MESSAGE_MAX_BYTES]) data to the ring buffer.
        inpOffset = 0;
        char* const inpPtr = &inpBuf[inpOffset];
        const int randomLength = MESSAGE_MAX_BYTES;
        const int inpBytes = (int) read_bin(inpFp, inpPtr, randomLength);
        if (0 == inpBytes) break;
        
        {
#define CMPBUFSIZE (LZ4_COMPRESSBOUND(MESSAGE_MAX_BYTES))
            char cmpBuf[CMPBUFSIZE];
            const int cmpBytes = LZ4_compress_fast_continue(lz4Stream, inpPtr, cmpBuf, inpBytes, CMPBUFSIZE, 0);
            if(cmpBytes <= 0) break;
            write_int32(outFp, cmpBytes);
            write_bin(outFp, cmpBuf, cmpBytes);
            
            LZ4_saveDict(lz4Stream, dict, dictSize);
            
            inpOffset += inpBytes;
            
            // Wraparound the ringbuffer offset
            if(inpOffset >= RING_BUFFER_BYTES - MESSAGE_MAX_BYTES) inpOffset = 0;
        }
    }
    
    write_int32(outFp, 0);
}


void test_decompress(FILE* outFp, FILE* inpFp)
{
    static char decBuf[DECODE_RING_BUFFER];
    int   decOffset    = 0;
    LZ4_streamDecode_t lz4StreamDecode_body = { 0 };
    LZ4_streamDecode_t* lz4StreamDecode = &lz4StreamDecode_body;
    
    for(;;) {
        int cmpBytes = 0;
        char cmpBuf[CMPBUFSIZE];
        
        {
            const size_t r0 = read_int32(inpFp, &cmpBytes);
            if(r0 != 1 || cmpBytes <= 0) break;
            
            const size_t r1 = read_bin(inpFp, cmpBuf, cmpBytes);
            if(r1 != (size_t) cmpBytes) break;
        }
        
        {
#if 0
            char* const decPtr = &decBuf[decOffset];
            const int decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decPtr, cmpBytes, MESSAGE_MAX_BYTES);
            if(decBytes <= 0) {
                break;
            }
            decOffset += decBytes;
            write_bin(outFp, decPtr, decBytes);
            
            // Wraparound the ringbuffer offset
            if(decOffset >= DECODE_RING_BUFFER - MESSAGE_MAX_BYTES) decOffset = 0;

#else
            char* const decPtr = &decBuf[decOffset];
            int decBytes = 0;
            int oldDecBytes = 0;
            int maxOutputSize = MESSAGE_MAX_BYTES / 16;
            
            while(true) {
                decBytes = LZ4_decompress_safe_continue(lz4StreamDecode, cmpBuf, decPtr, cmpBytes, maxOutputSize);
                if (decBytes <= 0) {
                    if (oldDecBytes == decBytes) {
                        goto _block_done; // decompression failed and we are not making any progress
                    } else {
                        maxOutputSize *= 2;
                        maxOutputSize += (1 << 16);
                        if (maxOutputSize > MESSAGE_MAX_BYTES)
                            maxOutputSize = MESSAGE_MAX_BYTES;
                        oldDecBytes = decBytes;
                    }
                } else {
                    decOffset += decBytes;
                    write_bin(outFp, decPtr, decBytes);
                    
                    // Wraparound the ringbuffer offset
                    if(decOffset >= DECODE_RING_BUFFER - MESSAGE_MAX_BYTES) decOffset = 0;
                    goto _block_done;

                }
            }
        _block_done:;
#endif

        }
    }
}


int compare(FILE* f0, FILE* f1)
{
    int result = 0;
    
    while(0 == result) {
        char b0[65536];
        char b1[65536];
        const size_t r0 = fread(b0, 1, sizeof(b0), f0);
        const size_t r1 = fread(b1, 1, sizeof(b1), f1);
        
        result = (int) r0 - (int) r1;
        
        if(0 == r0 || 0 == r1) {
            break;
        }
        if(0 == result) {
            result = memcmp(b0, b1, r0);
        }
    }
    
    return result;
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

bool testDictContent() {
    std::string data;
    std::stringstream ss;
    for (size_t i = 0; i < 10000; i++) {
        ss << std::setfill('0') << std::setw(10) << i;
    }
    data = ss.str();
    
    std::string md5New, md5Old;
    void* ctx = umundo::Message::createCompression();
    for (size_t i = 0; i < 100; i++) {
        umundo::Message msg(&data[i*10], 100);
        char compressed[1000];
        msg.compress("lz4", ctx, compressed, 1000, umundo::Message::PAYLOAD);
        comprCtxLZ4* lz4Ctx = (comprCtxLZ4*)ctx;
        md5New = md5(std::string(lz4Ctx->dataDict, lz4Ctx->dataDictSize));
        if (md5New != md5Old) {
            std::cout << "!";
            md5Old = md5New;
        }
    }
    
    return true;
}

int main(int argc, char** argv)
{
    testDictContent();
    exit(0);
    
    char inpFilename[256] = { 0 };
    char lz4Filename[256] = { 0 };
    char decFilename[256] = { 0 };
    
    snprintf(inpFilename, 256, "%s", argv[0]);
    snprintf(lz4Filename, 256, "%s.lz4s-%d", argv[0], 0);
    snprintf(decFilename, 256, "%s.lz4s-%d.dec", argv[0], 0);
    
    printf("inp = [%s]\n", inpFilename);
    printf("lz4 = [%s]\n", lz4Filename);
    printf("dec = [%s]\n", decFilename);
    
    // compress
    {
        FILE* inpFp = fopen(inpFilename, "rb");
        FILE* outFp = fopen(lz4Filename, "wb");
        
        test_compress(outFp, inpFp);
        
        fclose(outFp);
        fclose(inpFp);
    }
    
    // decompress
    {
        FILE* inpFp = fopen(lz4Filename, "rb");
        FILE* outFp = fopen(decFilename, "wb");
        
        test_decompress(outFp, inpFp);
        
        fclose(outFp);
        fclose(inpFp);
    }
    
    // verify
    {
        FILE* inpFp = fopen(inpFilename, "rb");
        FILE* decFp = fopen(decFilename, "rb");
        
        const int cmp = compare(inpFp, decFp);
        if(0 == cmp) {
            printf("Verify : OK\n");
        } else {
            printf("Verify : NG\n");
        }
        
        fclose(decFp);
        fclose(inpFp);
    }
    
    testDictContent();
    return 0;
}
