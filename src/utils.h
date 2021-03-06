#include <stdio.h>
#include <stdint.h>

#ifndef __INCL_UTILS
#define __INCL_UTILS

uint32_t    getFileSize(FILE * fptr);
char *      getFileExtension(char * pszFilename);
void        wipeBuffer(void * b, uint32_t bufferLen);
void        secureFree(void * b, uint32_t len);
void        dbg_free(void * buffer, const char * pszFile, const int line);
int         __getch();
void        hexDump(void * buffer, uint32_t bufferLen);
void        xorBuffer(uint8_t * target, uint8_t * source, size_t length);

#endif
