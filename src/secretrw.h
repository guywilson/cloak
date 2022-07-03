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

HSECRW      rdr_open(char * pszFilename, encryption_algo a);
int 		rdr_encrypt_aes256(HSECRW hsec, uint8_t * key, uint32_t keyLength);
int         rdr_encrypt_xor(HSECRW hsec, char * pszKeystreamFilename);
void        rdr_close(HSECRW hsec);
uint32_t    rdr_get_encrypted_buffer_length(HSECRW hsec);
uint32_t    rdr_get_data_length(HSECRW hsec);
uint32_t    rdr_get_file_length(HSECRW hsec);
uint32_t 	rdr_read_encrypted_data(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength);

HSECRW 		wrtr_open(char * pszFilename, encryption_algo a);
void 		wrtr_close(HSECRW hsec);
uint32_t 	wrtr_get_data_length(HSECRW hsec);
uint32_t 	wrtr_get_encryption_buffer_length(HSECRW hsec);
int 		wrtr_set_keystream_file(HSECRW hsec, char * pszFilename);
int 		wrtr_set_key_aes(HSECRW hsec, uint8_t * key, uint32_t keyLength);
uint32_t 	wrtr_get_header_length();
void 		wrtr_read_header(HSECRW hsec, uint8_t * headerBuffer, uint32_t headerBufferLength);
int 		wrtr_write_decrypted_data(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength);

#endif
