#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_READER
#define __INCL_READER

HCLOAK      rdr_open(char * pszFilename, uint32_t blockSize, encryption_algo a);
void        rdr_close(HCLOAK hc);
uint32_t    rdr_get_block_size(HCLOAK hc);
uint32_t    rdr_read_block(HCLOAK hc, uint8_t * buffer);

#endif
