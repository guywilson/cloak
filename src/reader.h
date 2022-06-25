#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_READER
#define __INCL_READER

struct _cloak_handle;
typedef struct _cloak_handle * HCLOAK;

HCLOAK      rdr_open(char * pszFilename, uint8_t * key, uint32_t keyLength, uint32_t blockSize, encryption_algo a);
void        rdr_close(HCLOAK hc);
uint32_t    rdr_get_block_size(HCLOAK hc);
uint32_t    rdr_get_data_length(HCLOAK hc);
uint32_t    rdr_get_file_length(HCLOAK hc);
boolean     rdr_has_more_blocks(HCLOAK hc);
uint32_t    rdr_read_block(HCLOAK hc, uint8_t * buffer);

#endif
