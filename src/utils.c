#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#endif

#include "random_block.h"

//#define __DEBUG_MEM

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

char * getFileExtension(char * pszFilename)
{
	char *			pszExt = NULL;
	int				i;
	
	i = strlen(pszFilename);

	while (i > 0) {
		if (pszFilename[i] == '.') {
			pszExt = &pszFilename[i + 1];
			break;
		}
		
		i--;
	}

	return pszExt;		
}

void hexDump(void * buffer, uint32_t bufferLen)
{
    int         i;
    int         j = 0;
    uint8_t *   buf;
    static char szASCIIBuf[17];

    buf = (uint8_t *)buffer;

    for (i = 0;i < bufferLen;i++) {
        if ((i % 16) == 0) {
            if (i != 0) {
                szASCIIBuf[j] = 0;
                j = 0;

                printf("  |%s|", szASCIIBuf);
            }
                
            printf("\n%08X\t", i);
        }

        if ((i % 2) == 0 && (i % 16) > 0) {
            printf(" ");
        }

        printf("%02X", buf[i]);
        szASCIIBuf[j++] = isprint(buf[i]) ? buf[i] : '.';
    }

    /*
    ** Print final ASCII block...
    */
    szASCIIBuf[j] = 0;
    printf("  |%s|\n", szASCIIBuf);
}

int __getch()
{
	int		ch;

#ifndef _WIN32
	struct termios current;
	struct termios original;

	tcgetattr(fileno(stdin), &original); /* grab old terminal i/o settings */
	current = original; /* make new settings same as old settings */
	current.c_lflag &= ~ICANON; /* disable buffered i/o */
	current.c_lflag &= ~ECHO; /* set echo mode */
	tcsetattr(fileno(stdin), TCSANOW, &current); /* use these new terminal i/o settings now */
#endif

#ifdef _WIN32
    ch = _getch();
#else
    ch = getchar();
#endif

#ifndef _WIN32
	tcsetattr(0, TCSANOW, &original);
#endif

    return ch;
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

void secureFree(void * b, uint32_t len)
{
    wipeBuffer(b, len);
    free(b);
}

void dbg_free(void * buffer, const char * pszFile, const int line)
{
#ifdef __DEBUG_MEM
    uint64_t        address;

    address = (uint64_t)buffer;

    printf("Called dbg_free() from %s:%d, free(ing) buffer at address: 0x%llU\n", pszFile, line, address);
#endif
    free(buffer);
}

void xorBuffer(uint8_t * target, uint8_t * source, size_t length)
{
    uint32_t        i;

    for (i = 0;i < length;i++) {
        target[i] = target[i] ^ source[i];
    }
}
