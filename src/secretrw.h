#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_READER
#define __INCL_READER

struct _secret_rw_handle;
typedef struct _secret_rw_handle * HSECRW;

typedef enum {
	xor,
	aes256,
	none
}
encryption_algo;

HSECRW      rdr_open(char * pszFilename, uint8_t * key, uint32_t keyLength, uint32_t blockSize, encryption_algo a);
int         rdr_set_keystream_file(HSECRW hsec, char * pszKeystreamFilename);
void        rdr_close(HSECRW hsec);
uint32_t    rdr_get_block_size(HSECRW hsec);
uint32_t    rdr_get_data_length(HSECRW hsec);
uint32_t    rdr_get_file_length(HSECRW hsec);
boolean     rdr_has_more_blocks(HSECRW hsec);
uint32_t    rdr_read_block(HSECRW hsec, uint8_t * buffer);

#endif
