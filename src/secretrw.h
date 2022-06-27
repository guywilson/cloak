#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_READER
#define __INCL_READER

struct _secret_rdr_handle;
typedef struct _secret_rdr_handle * HSECRDR;

typedef enum {
	xor,
	aes256,
	none
}
encryption_algo;

HSECRDR      rdr_open(char * pszFilename, uint8_t * key, uint32_t keyLength, uint32_t blockSize, encryption_algo a);
int         rdr_set_keystream_file(HSECRDR hsec, char * pszKeystreamFilename);
void        rdr_close(HSECRDR hsec);
uint32_t    rdr_get_block_size(HSECRDR hsec);
uint32_t    rdr_get_data_length(HSECRDR hsec);
uint32_t    rdr_get_file_length(HSECRDR hsec);
boolean     rdr_has_more_blocks(HSECRDR hsec);
uint32_t    rdr_read_block(HSECRDR hsec, uint8_t * buffer);

#endif
