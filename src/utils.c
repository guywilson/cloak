#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "random_block.h"

uint32_t getFileSize(FILE * fptr)
{
	uint32_t                size;
	long                    currentPos;

	currentPos = ftell(fptr);
	
	fseek(fptr, 0L, SEEK_SET);
	fseek(fptr, 0L, SEEK_END);
	
	size = (uint32_t)ftell(fptr);

	fseek(fptr, currentPos, SEEK_SET);

	return size;
}

void wipeBuffer(void * b, uint32_t bufferLen)
{
    uint32_t        c;
    uint32_t        i = 0;
    uint8_t *       buffer;

    memset(b, 0x00, bufferLen);
    memset(b, 0xFF, bufferLen);
    memset(b, 0x00, bufferLen);
    memset(b, 0xFF, bufferLen);
    memset(b, 0x00, bufferLen);

    buffer = (uint8_t *)b;

    for (c = 0;c < bufferLen;c++) {
        buffer[c] = random_block[i++];

        if (i == MAX_BLOCK_SIZE) {
            i = 0;
        }
    }

    memset(b, 0x00, bufferLen);
}

void xorBuffer(void * target, void * source, size_t length)
{
    uint32_t        c;
    uint8_t *       t;
    uint8_t *       s;

    t = (uint8_t *)target;
    s = (uint8_t *)source;

    for (c = 0;c < length;c++) {
        t[c] = t[c] ^ s[c];
    }
}
