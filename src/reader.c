#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cloak_types.h"

uint32_t readBlock(FILE * f, uint32_t length, uint8_t * buffer)
{
	uint32_t			bytesRead;
	boolean				isFirstBlock = false;

	isFirstBlock = (ftell(f) == 0) ? true : false;

	if (isFirstBlock) {

	}
	
	return bytesRead;
}
