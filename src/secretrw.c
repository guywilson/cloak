#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <gcrypt.h>

#include "cloak_types.h"
#include "random_block.h"
#include "utils.h"
#include "secretrw.h"

#define MAX_FILE_SIZE					67108864			// 64Mb

struct _secret_rdr_handle {
	encryption_algo		algo;
	uint8_t *			data;
	uint32_t			blockSize;
	uint32_t			fileLength;
	uint32_t			dataFrameLength;
	uint32_t			encryptionBufferLength;

	uint32_t			blockCounter;
	uint32_t			counter;

	FILE *				fptrInput;
	FILE *				fptrKey;

	gcry_cipher_hd_t	cipherHandle;
};

typedef struct __attribute__((__packed__))
{
    uint32_t        fileLength;
    uint32_t        dataFrameLength;
	uint32_t		encryptionBufferLength;
	uint8_t			padding[4];
}
CLOAK_HEADER;

HSECRDR rdr_open(char * pszFilename, uint8_t * key, uint32_t keyLength, uint32_t blockSize, encryption_algo a)
{
	HSECRDR			hsec;

	if (blockSize == 0) {
		fprintf(stderr, "Block size must be greater than 0\n");
		return NULL;
	}
	else if (blockSize > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Specified block size %u is greater than maximum allowed (%u)\n", blockSize, MAX_BLOCK_SIZE);
		return NULL;
	}

	hsec = (HSECRDR)malloc(sizeof(struct _secret_rdr_handle));

	if (hsec == NULL) {
		fprintf(stderr, "Failed to allocate memory for cloak handle\n");
		return NULL;
	}

	hsec->blockSize = blockSize;
	hsec->algo = a;
	hsec->counter = 0;
	hsec->blockCounter = 0;

	hsec->fptrKey = NULL;

	hsec->fptrInput = fopen(pszFilename, "rb");

	if (hsec->fptrInput == NULL) {
		fprintf(stderr, "Failed to open file reader with file %s: %s\n", pszFilename, strerror(errno));
		return NULL;
	}

	hsec->fileLength = getFileSize(hsec->fptrInput);

	/*
	** The AES-256 data frame consists of:
	**
	** 1. 16-byte header with filelength & datalength
	** 2. IV block, typically 128-bit, 16 bytes
	** 3. Encryted data n blocks long
	*/
	if (hsec->algo == aes256) {
		int				err;
		int				index = 0;
		uint32_t		blklen;
		uint32_t		bytesRead;
		uint8_t *		iv;
		CLOAK_HEADER	header;

		if (hsec->fileLength > MAX_FILE_SIZE) {
			fprintf(stderr, "File length %u is over the maximum allowed\n", hsec->fileLength);
			fclose(hsec->fptrInput);
			return NULL;
		}

		err = gcry_cipher_open(
							&hsec->cipherHandle,
							GCRY_CIPHER_RIJNDAEL256,
							GCRY_CIPHER_MODE_CBC,
							0);

		if (err) {
			fprintf(stderr, "Failed to open cipher with gcrypt\n");
			fclose(hsec->fptrInput);
			free(hsec);
			return NULL;
		}

		blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

		err = gcry_cipher_setkey(
							hsec->cipherHandle,
							(const void *)key,
							keyLength);

		if (err) {
			fprintf(stderr, "Failed to set key with gcrypt: %s/%s\n", gcry_strerror(err), gcry_strsource(err));
			fclose(hsec->fptrInput);
			free(hsec);
			return NULL;
		}

		iv = (uint8_t *)malloc(blklen);

		if (iv == NULL) {
			fprintf(stderr, "Failed to allocate memory for IV of size %u\n", blockSize);
			fclose(hsec->fptrInput);
			free(hsec);
			return NULL;
		}

		gcry_randomize(iv, blklen, GCRY_STRONG_RANDOM);

		err = gcry_cipher_setiv(
							hsec->cipherHandle,
							iv,
							blklen);

		if (err) {
			fprintf(stderr, "Failed to set IV with gcrypt\n");
			free(iv);
			fclose(hsec->fptrInput);
			free(hsec);
			return NULL;
		}

		hsec->encryptionBufferLength = hsec->fileLength + (blklen - (hsec->fileLength % blklen));
		hsec->dataFrameLength = hsec->encryptionBufferLength + sizeof(CLOAK_HEADER) + blklen;

		hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

		if (hsec->data == NULL) {
			fprintf(stderr, "Failed to allocate memory for data of size %u\n", hsec->dataFrameLength);
			free(iv);
			fclose(hsec->fptrInput);
			free(hsec);
			return NULL;
		}

		header.fileLength = hsec->fileLength;
		header.dataFrameLength = hsec->dataFrameLength;
		header.encryptionBufferLength = hsec->encryptionBufferLength;

		/*
		** XOR the header with random data...
		*/
		xorBuffer(&header, &random_block[2048], sizeof(CLOAK_HEADER));

		memcpy(&hsec->data[index], &header, sizeof(CLOAK_HEADER));
		index += sizeof(CLOAK_HEADER);

		memcpy(&hsec->data[index], iv, blklen);
		index += blklen;

		free(iv);

		bytesRead = fread(&hsec->data[index], 1, hsec->fileLength, hsec->fptrInput);

		if (bytesRead < hsec->fileLength) {
			fprintf(stderr, "Failed to read file %s, expected %u bytes, got %u bytes\n", pszFilename, hsec->fileLength, bytesRead);
			fclose(hsec->fptrInput);
			free(hsec->data);
			free(hsec);
			return NULL;
		}

		fclose(hsec->fptrInput);
		hsec->fptrInput = NULL;

		/*
		** Fill any remaining bytes with random data...
		*/
		memcpy(
			&hsec->data[index + hsec->fileLength], 
			random_block, 
			(hsec->dataFrameLength - hsec->fileLength - index));

		err = gcry_cipher_encrypt(
					hsec->cipherHandle, 
					&hsec->data[index], 
					hsec->encryptionBufferLength, 
					NULL, 
					0);

		if (err) {
			fprintf(stderr, "Failed to encrypt with gcrypt\n");
			free(hsec);
			return NULL;
		}

		gcry_cipher_close(hsec->cipherHandle);
	}
	else {
		hsec->encryptionBufferLength = hsec->fileLength + (hsec->blockSize - (hsec->fileLength % hsec->blockSize));
	}

	return hsec;
}

int rdr_set_keystream_file(HSECRDR hsec, char * pszKeystreamFilename)
{
	uint32_t		keyLength;

	hsec->fptrKey = fopen(pszKeystreamFilename, "rb");

	if (hsec->fptrKey == NULL) {
		fprintf(stderr, "Failed to open keystream file %s: %s\n", pszKeystreamFilename, strerror(errno));
		return -1;
	}

	keyLength = getFileSize(hsec->fptrKey);

	if (keyLength < hsec->dataFrameLength) {
		fprintf(stderr, "Keystream file must be at least %u bytes long\n", hsec->dataFrameLength);
		fclose(hsec->fptrKey);
		return -1;
	}

	return 0;
}

void rdr_close(HSECRDR hsec)
{
	if (hsec->fptrInput != NULL) {
		fclose(hsec->fptrInput);
	}

	if (hsec->fptrKey != NULL) {
		fclose(hsec->fptrKey);
	}

	//free(hsec->data);
	free(hsec);
}

uint32_t rdr_get_block_size(HSECRDR hsec)
{
	return hsec->blockSize;
}

uint32_t rdr_get_data_length(HSECRDR hsec)
{
	return hsec->dataFrameLength;
}

uint32_t rdr_get_file_length(HSECRDR hsec)
{
	return hsec->fileLength;
}

boolean rdr_has_more_blocks(HSECRDR hsec)
{
	return (hsec->counter < hsec->dataFrameLength) ? true : false;
}

uint32_t rdr_read_block(HSECRDR hsec, uint8_t * buffer)
{
	uint32_t			bytesRead;

	memcpy(buffer, random_block, hsec->blockSize);

	if (hsec->algo == aes256) {
		bytesRead = ((hsec->dataFrameLength - hsec->counter) >= hsec->blockSize ? hsec->blockSize : (hsec->dataFrameLength - hsec->counter));
		
		memcpy(buffer, hsec->data, bytesRead);
		hsec->data += bytesRead;
	}
	else {
		if (hsec->blockCounter == 0) {
			CLOAK_HEADER	header;

			header.fileLength = hsec->fileLength;
			header.dataFrameLength = hsec->dataFrameLength;

			memcpy(buffer, &header, sizeof(CLOAK_HEADER));

			bytesRead = sizeof(CLOAK_HEADER);
			bytesRead += fread(&buffer[sizeof(CLOAK_HEADER)], 1, (hsec->blockSize - sizeof(CLOAK_HEADER)), hsec->fptrInput);
		}
		else {
			bytesRead = fread(buffer, 1, hsec->blockSize, hsec->fptrInput);
		}

		if (hsec->algo == xor) {
			uint32_t		index = 0;

			while (index < hsec->blockSize) {
				buffer[index] = buffer[index] ^ (uint8_t)fgetc(hsec->fptrKey);
				index++;
			}
		}
	}

	hsec->blockCounter++;
	hsec->counter += bytesRead;

	return bytesRead;
}
