#include <stdio.h>
#include <stdint.h>

#ifndef __INCL_UTILS
#define __INCL_UTILS

int         generateKeystreamFile(const char * pszKeystreamFile, uint32_t numBytes);
uint32_t    getFileSize(FILE * fptr);
uint32_t    getFileSizeByName(const char * pszFilename);
char *      getFileExtension(char * pszFilename);
void        wipeBuffer(void * b, uint32_t bufferLen);
void        secureFree(void * b, uint32_t len);
void *      dbg_malloc(uint16_t id, size_t numBytes, const char * pszFile, const int line);
void        dbg_free(uint16_t id, void * buffer, const char * pszFile, const int line);
int         __getch(void);
void        hexDump(void * buffer, uint32_t bufferLen);
void        xorBuffer(uint8_t * target, uint8_t * source, size_t length);

#endif
