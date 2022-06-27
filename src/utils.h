#include <stdio.h>
#include <stdint.h>

#ifndef __INCL_UTILS
#define __INCL_UTILS

uint32_t    getFileSize(FILE * fptr);
void        wipeBuffer(void * b, uint32_t bufferLen);
void        secureFree(void * b, uint32_t len);
void        xorBuffer(void * target, void * source, size_t length);

#endif
