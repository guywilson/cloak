#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <gcrypt.h>

#include "cloak_types.h"
#include "random_block.h"
#include "reader.h"

struct _cloak_handle {
	encryption_algo		algo;
	uint8_t *			key;
	uint32_t			blockSize;

	FILE *				fptrInput;
};

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

HCLOAK rdr_open(char * pszFilename, uint32_t blockSize, encryption_algo a)
{
	HCLOAK			hc;

	hc = (HCLOAK)malloc(sizeof(struct _cloak_handle));

	if (hc == NULL) {
		fprintf(stderr, "Failed to allocate memory for cloak handle\n");
		return NULL;
	}

	if (a == aes256) {
		hc->blockSize = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);
	}
	else if (a == xor) {
		hc->blockSize = 512;
	}
	else {
		if (blockSize == 0) {
			fprintf(stderr, "Block size must be greater than 0\n");
			return NULL;
		}
		else if (blockSize > MAX_BLOCK_SIZE) {
			fprintf(stderr, "Specified block size %u is greater than maximum allowed (%u)\n", blockSize, MAX_BLOCK_SIZE);
			return NULL;
		}

		hc->blockSize = blockSize;
	}

	hc->algo = a;

	hc->fptrInput = fopen(pszFilename, "rb");

	if (hc->fptrInput == NULL) {
		fprintf(stderr, "Failed to open file reader with file %s: %s\n", pszFilename, strerror(errno));
		return NULL;
	}

	return hc;
}

void rdr_close(HCLOAK hc)
{
	fclose(hc->fptrInput);
	free(hc);
}

uint32_t rdr_get_block_size(HCLOAK hc)
{
	return hc->blockSize;
}

uint32_t rdr_read_block(HCLOAK hc, uint8_t * buffer)
{
	uint32_t			bytesRead;
	uint32_t			dataLength;
	boolean				isFirstBlock = false;

	isFirstBlock = (ftell(hc->fptrInput) == 0) ? true : false;

	dataLength = getFileSize(hc->fptrInput);
	dataLength += (hc->blockSize - (dataLength % hc->blockSize));

	memcpy(buffer, random_block, hc->blockSize);

	if (isFirstBlock) {
		memcpy(buffer, &dataLength, sizeof(uint32_t));

		bytesRead = sizeof(uint32_t);
		bytesRead += fread(&buffer[sizeof(uint32_t)], 1, (hc->blockSize - sizeof(uint32_t)), hc->fptrInput);
	}
	else {
		bytesRead = fread(buffer, 1, hc->blockSize, hc->fptrInput);
	}

	return bytesRead;
}
