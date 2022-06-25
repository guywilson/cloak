#include <stdio.h>

#ifndef __INCL_CLOAK_TYPES
#define __INCL_CLOAK_TYPES

typedef enum {
	false = 0,
	true = 1
}
boolean;

typedef enum {
	xor,
	aes256,
	none
}
encryption_algo;

typedef struct __attribute__((__packed__))
{
    uint32_t        crc;
    uint32_t        originalLength;
    uint32_t        encryptedLength;
}
CLOAK_HEADER;

struct _cloak_handle;
typedef struct _cloak_handle * HCLOAK;

#endif
