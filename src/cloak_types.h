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

#endif
