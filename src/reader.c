#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <gcrypt.h>

#include "cloak_types.h"
#include "random_block.h"
#include "reader.h"

#define MAX_FILE_SIZE					67108864			// 64Mb

struct _cloak_handle {
	encryption_algo		algo;
	uint8_t *			data;
	uint32_t			blockSize;
	uint32_t			fileLength;
	uint32_t			dataLength;

	uint32_t			blockCounter;
	uint32_t			counter;

	FILE *				fptrInput;
	FILE *				fptrKey;

	gcry_cipher_hd_t	cipherHandle;
};

typedef struct __attribute__((__packed__))
{
    uint32_t        fileLength;
    uint32_t        dataLength;
}
CLOAK_HEADER;

uint32_t getFileSize(FILE * fptr)
{
	uint32_t                size;
	long                    currentPos;

	currentPos = ftell(fptr);
	
	fseek(fptr, 0L, SEEK_SET);
	fseek(fptr, 0L, SEEK_END);
	
	size = (uint32_t)ftell(fptr);

	fseek(fptr, currentPos, SEEK_SET);

	return size;
}

HCLOAK rdr_open(char * pszFilename, uint8_t * key, uint32_t keyLength, uint32_t blockSize, encryption_algo a)
{
	HCLOAK			hc;
	uint32_t		rawDataLength;

	if (blockSize == 0) {
		fprintf(stderr, "Block size must be greater than 0\n");
		return NULL;
	}
	else if (blockSize > MAX_BLOCK_SIZE) {
		fprintf(stderr, "Specified block size %u is greater than maximum allowed (%u)\n", blockSize, MAX_BLOCK_SIZE);
		return NULL;
	}

	hc = (HCLOAK)malloc(sizeof(struct _cloak_handle));

	if (hc == NULL) {
		fprintf(stderr, "Failed to allocate memory for cloak handle\n");
		return NULL;
	}

	hc->blockSize = blockSize;
	hc->algo = a;
	hc->counter = 0;
	hc->blockCounter = 0;


	hc->fptrInput = fopen(pszFilename, "rb");

	if (hc->fptrInput == NULL) {
		fprintf(stderr, "Failed to open file reader with file %s: %s\n", pszFilename, strerror(errno));
		return NULL;
	}

	hc->fileLength = getFileSize(hc->fptrInput);

	rawDataLength = hc->fileLength + sizeof(CLOAK_HEADER);

	if (hc->algo == aes256) {
		int				err;
		int				index = 0;
		uint32_t		blklen;
		uint32_t		bytesRead;
		uint8_t *		iv;
		CLOAK_HEADER	header;

		if (hc->fileLength > MAX_FILE_SIZE) {
			fprintf(stderr, "File length %u is over the maximum allowed\n", hc->fileLength);
			fclose(hc->fptrInput);
			return NULL;
		}

		err = gcry_cipher_open(
							&hc->cipherHandle,
							GCRY_CIPHER_RIJNDAEL256,
							GCRY_CIPHER_MODE_CBC,
							0);

		if (err) {
			fprintf(stderr, "Failed to open cipher with gcrypt\n");
			fclose(hc->fptrInput);
			free(hc);
			return NULL;
		}

		blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

		err = gcry_cipher_setkey(
							hc->cipherHandle,
							(const void *)key,
							keyLength);

		if (err) {
			fprintf(stderr, "Failed to set key with gcrypt\n");
			fclose(hc->fptrInput);
			free(hc);
			return NULL;
		}

		iv = (uint8_t *)malloc(blklen);

		if (iv == NULL) {
			fprintf(stderr, "Failed to allocate memory for IV of size %u\n", blockSize);
			fclose(hc->fptrInput);
			free(hc);
			return NULL;
		}

		gcry_randomize(iv, blklen, GCRY_STRONG_RANDOM);

		err = gcry_cipher_setiv(
							hc->cipherHandle,
							iv,
							blklen);

		if (err) {
			fprintf(stderr, "Failed to set IV with gcrypt\n");
			free(iv);
			fclose(hc->fptrInput);
			free(hc);
			return NULL;
		}

		/*
		** With AES256, the first block of data has to be an IV block
		** which we generate randomly...
		*/
		rawDataLength += blklen;

		hc->dataLength = hc->fileLength + (hc->blockSize - (rawDataLength % hc->blockSize));

		hc->data = (uint8_t *)malloc(hc->dataLength);

		if (hc->data == NULL) {
			fprintf(stderr, "Failed to allocate memory for data of size %u\n", hc->dataLength);
			free(iv);
			fclose(hc->fptrInput);
			free(hc);
			return NULL;
		}

		memcpy(hc->data, iv, blklen);
		index += blklen;

		free(iv);

		header.fileLength = hc->fileLength;
		header.dataLength = hc->dataLength;

		memcpy(&hc->data[index], &header, sizeof(CLOAK_HEADER));
		index += sizeof(CLOAK_HEADER);

		bytesRead = fread(&hc->data[index], 1, hc->fileLength, hc->fptrInput);

		if (bytesRead < hc->fileLength) {
			fprintf(stderr, "Failed to read file %s, expected %u bytes, got %u bytes\n", pszFilename, hc->fileLength, bytesRead);
			fclose(hc->fptrInput);
			free(hc->data);
			free(hc);
			return NULL;
		}

		fclose(hc->fptrInput);

		err = gcry_cipher_encrypt(hc->cipherHandle, hc->data, hc->dataLength, NULL, 0);

		if (err) {
			fprintf(stderr, "Failed to set encrypt with gcrypt\n");
			free(hc);
			return NULL;
		}

		gcry_cipher_close(hc->cipherHandle);
	}
	else {
		hc->dataLength = hc->fileLength + (hc->blockSize - (rawDataLength % hc->blockSize));
	}

	return hc;
}

int rdr_set_keystream_file(HCLOAK hc, char * pszKeystreamFilename)
{
	uint32_t		keyLength;

	hc->fptrKey = fopen(pszKeystreamFilename, "rb");

	if (hc->fptrKey == NULL) {
		fprintf(stderr, "Failed to open keystream file %s: %s\n", pszKeystreamFilename, strerror(errno));
		return -1;
	}

	keyLength = getFileSize(hc->fptrKey);

	if (keyLength < hc->dataLength) {
		fprintf(stderr, "Keystream file must be at least %u bytes long\n", hc->dataLength);
		fclose(hc->fptrKey);
		return -1;
	}

	return 0;
}

void rdr_close(HCLOAK hc)
{
	fclose(hc->fptrInput);

	if (hc->fptrKey != NULL) {
		fclose(hc->fptrKey);
	}

	free(hc->data);
	free(hc);
}

uint32_t rdr_get_block_size(HCLOAK hc)
{
	return hc->blockSize;
}

uint32_t rdr_get_data_length(HCLOAK hc)
{
	return hc->dataLength;
}

uint32_t rdr_get_file_length(HCLOAK hc)
{
	return hc->fileLength;
}

boolean rdr_has_more_blocks(HCLOAK hc)
{
	return (hc->counter < hc->dataLength) ? true : false;
}

uint32_t rdr_read_block(HCLOAK hc, uint8_t * buffer)
{
	uint32_t			bytesRead;

	memcpy(buffer, random_block, hc->blockSize);

	if (hc->algo == aes256) {
		memcpy(buffer, hc->data, hc->blockSize);
		hc->data += hc->blockSize;

		bytesRead = hc->blockSize;
	}
	else {
		if (hc->blockCounter == 0) {
			CLOAK_HEADER	header;

			header.fileLength = hc->fileLength;
			header.dataLength = hc->dataLength;

			memcpy(buffer, &header, sizeof(CLOAK_HEADER));

			bytesRead = sizeof(CLOAK_HEADER);
			bytesRead += fread(&buffer[sizeof(CLOAK_HEADER)], 1, (hc->blockSize - sizeof(CLOAK_HEADER)), hc->fptrInput);
		}
		else {
			bytesRead = fread(buffer, 1, hc->blockSize, hc->fptrInput);
		}

		if (hc->algo == xor) {
			uint32_t		index = 0;

			while (index < hc->blockSize) {
				buffer[index] = buffer[index] ^ (uint8_t)fgetc(hc->fptrKey);
				index++;
			}
		}
	}

	hc->blockCounter++;
	hc->counter += bytesRead;

	return bytesRead;
}
