#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_READER
#define __INCL_READER

#define SECRETRW_BLOCK_SIZE				64

struct _secret_rw_handle;
typedef struct _secret_rw_handle * HSECRW;

typedef enum {
	xor,
	aes256,
	none
}
encryption_algo;

HSECRW      rdr_open(const char * pszFilename, encryption_algo a);
int 		rdr_encrypt_aes256(HSECRW hsec, uint8_t * key, uint32_t keyLength);
int         rdr_encrypt_xor(HSECRW hsec, const char * pszKeystreamFilename);
void        rdr_close(HSECRW hsec);
uint32_t    rdr_get_block_size(HSECRW hsec);
uint32_t    rdr_get_data_length(HSECRW hsec);
uint32_t    rdr_get_file_length(HSECRW hsec);
boolean     rdr_has_more_blocks(HSECRW hsec);
uint32_t 	rdr_read_encrypted_block(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength);

HSECRW 		wrtr_open(const char * pszFilename, encryption_algo a);
void 		wrtr_close(HSECRW hsec);
uint32_t 	wrtr_get_block_size(HSECRW hsec);
boolean 	wrtr_has_more_blocks(HSECRW hsec);
int 		wrtr_set_keystream_file(HSECRW hsec, const char * pszFilename);
int 		wrtr_set_key_aes(HSECRW hsec, uint8_t * key, uint32_t keyLength);
int 		wrtr_write_decrypted_block(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength);

#endif
