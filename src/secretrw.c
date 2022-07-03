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
#define BLOCK_SIZE						64;

struct _secret_rw_handle {
	encryption_algo		algo;
	uint8_t *			data;
	uint32_t			blockSize;
	uint32_t			fileLength;
	uint32_t			dataFrameLength;
	uint32_t			encryptionBufferLength;

	uint32_t			blockCounter;
	uint32_t			numBlocks;
	uint32_t			counter;

	FILE *				fptrSecret;
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


HSECRW rdr_open(char * pszFilename, encryption_algo a)
{
	HSECRW			hsec;
	CLOAK_HEADER	header;
	int				index = 0;
	uint32_t		bytesRead;

	hsec = (HSECRW)malloc(sizeof(struct _secret_rw_handle));

	if (hsec == NULL) {
		fprintf(stderr, "Failed to allocate memory for cloak handle\n");
		return NULL;
	}

	hsec->blockSize = BLOCK_SIZE;
	hsec->algo = a;
	hsec->counter = 0;
	hsec->blockCounter = 0;

	hsec->fptrKey = NULL;

	hsec->fptrSecret = fopen(pszFilename, "rb");

	if (hsec->fptrSecret == NULL) {
		fprintf(stderr, "Failed to open file reader with file %s: %s\n", pszFilename, strerror(errno));
		return NULL;
	}

	hsec->fileLength = getFileSize(hsec->fptrSecret);

	if (hsec->fileLength > MAX_FILE_SIZE) {
		fprintf(stderr, "File length %u is over the maximum allowed\n", hsec->fileLength);
		fclose(hsec->fptrSecret);
		return NULL;
	}

	/*
	** The AES-256 data frame consists of:
	**
	** 1. 16-byte header with filelength & datalength
	** 2. IV block, typically 128-bit, 16 bytes
	** 3. Encryted data n blocks long
	*/
	if (hsec->algo == aes256) {
		int				err;
		uint32_t		blklen;
		uint8_t *		iv;

		err = gcry_cipher_open(
							&hsec->cipherHandle,
							GCRY_CIPHER_RIJNDAEL256,
							GCRY_CIPHER_MODE_CBC,
							0);

		if (err) {
			fprintf(stderr, "Failed to open cipher with gcrypt\n");
			fclose(hsec->fptrSecret);
			free(hsec);
			return NULL;
		}

		blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

		iv = (uint8_t *)malloc(blklen);

		if (iv == NULL) {
			fprintf(stderr, "Failed to allocate memory for IV of size %u\n", blklen);
			fclose(hsec->fptrSecret);
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
			fclose(hsec->fptrSecret);
			free(hsec);
			return NULL;
		}

		hsec->encryptionBufferLength = hsec->fileLength + (blklen - (hsec->fileLength % blklen));
		hsec->dataFrameLength = hsec->encryptionBufferLength + sizeof(CLOAK_HEADER) + blklen;

		hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

		if (hsec->data == NULL) {
			fprintf(stderr, "Failed to allocate memory for data of size %u\n", hsec->dataFrameLength);
			free(iv);
			fclose(hsec->fptrSecret);
			free(hsec);
			return NULL;
		}

		header.fileLength = hsec->fileLength;
		header.dataFrameLength = hsec->dataFrameLength;
		header.encryptionBufferLength = hsec->encryptionBufferLength;

		/*
		** XOR the header with random data...
		*/
		memcpy(&hsec->data[index], &header, sizeof(CLOAK_HEADER));
		xorBuffer(&hsec->data[index], &random_block[2048], sizeof(CLOAK_HEADER));

		index += sizeof(CLOAK_HEADER);

		memcpy(&hsec->data[index], iv, blklen);
		index += blklen;

		free(iv);

		bytesRead = fread(&hsec->data[index], 1, hsec->fileLength, hsec->fptrSecret);

		if (bytesRead < hsec->fileLength) {
			fprintf(stderr, "Failed to read file %s, expected %u bytes, got %u bytes\n", pszFilename, hsec->fileLength, bytesRead);
			fclose(hsec->fptrSecret);
			free(hsec->data);
			free(hsec);
			return NULL;
		}

		fclose(hsec->fptrSecret);
		hsec->fptrSecret = NULL;

		/*
		** Fill any remaining bytes with random data...
		*/
		memcpy(
			&hsec->data[index + hsec->fileLength], 
			random_block, 
			(hsec->dataFrameLength - hsec->fileLength - index));
	}
	else if (hsec->algo == xor) {
		hsec->encryptionBufferLength = hsec->fileLength + sizeof(CLOAK_HEADER);
		hsec->dataFrameLength = hsec->encryptionBufferLength;
		
		hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

		if (hsec->data == NULL) {
			fprintf(stderr, "Failed to allocate memory for data of size %u\n", hsec->dataFrameLength);
			fclose(hsec->fptrSecret);
			free(hsec);
			return NULL;
		}

		header.fileLength = hsec->fileLength;
		header.dataFrameLength = hsec->dataFrameLength;
		header.encryptionBufferLength = hsec->encryptionBufferLength;

		/*
		** XOR the header with random data...
		*/
		memcpy(&hsec->data[index], &header, sizeof(CLOAK_HEADER));
		xorBuffer(&hsec->data[index], &random_block[2048], sizeof(CLOAK_HEADER));

		index += sizeof(CLOAK_HEADER);

		bytesRead = fread(&hsec->data[index], 1, hsec->fileLength, hsec->fptrSecret);

		if (bytesRead < hsec->fileLength) {
			fprintf(stderr, "Failed to read file %s, expected %u bytes, got %u bytes\n", pszFilename, hsec->fileLength, bytesRead);
			fclose(hsec->fptrSecret);
			free(hsec->data);
			free(hsec);
			return NULL;
		}

		fclose(hsec->fptrSecret);
		hsec->fptrSecret = NULL;
	}
	else {
		hsec->encryptionBufferLength = hsec->fileLength;
		hsec->dataFrameLength = hsec->encryptionBufferLength;

		printf("No encryption: fileLen=%u, encryptionLen=%u, dataFrameLen=%u\n", hsec->fileLength, hsec->encryptionBufferLength, hsec->dataFrameLength);
	}

	return hsec;
}

int rdr_encrypt_aes256(HSECRW hsec, uint8_t * key, uint32_t keyLength)
{
	int			err;
	uint32_t	blklen;

	blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

	err = gcry_cipher_setkey(
						hsec->cipherHandle,
						(const void *)key,
						keyLength);

	if (err) {
		fprintf(stderr, "Failed to set key with gcrypt: %s/%s\n", gcry_strerror(err), gcry_strsource(err));
		fclose(hsec->fptrSecret);
		free(hsec);
		return -1;
	}

	err = gcry_cipher_encrypt(
				hsec->cipherHandle, 
				&hsec->data[sizeof(CLOAK_HEADER) + blklen], 
				hsec->encryptionBufferLength, 
				NULL, 
				0);

	if (err) {
		fprintf(stderr, "Failed to encrypt with gcrypt: %s\n", gcry_strerror(err));
		fclose(hsec->fptrSecret);
		free(hsec);
		return -1;
	}

	gcry_cipher_close(hsec->cipherHandle);
	
	return 0;
}

int rdr_encrypt_xor(HSECRW hsec, char * pszKeystreamFilename)
{
	uint32_t		keyLength;
	uint32_t		i;
	int				ch;

	hsec->fptrKey = fopen(pszKeystreamFilename, "rb");

	if (hsec->fptrKey == NULL) {
		fprintf(stderr, "Failed to open keystream file %s: %s\n", pszKeystreamFilename, strerror(errno));
		return -1;
	}

	keyLength = getFileSize(hsec->fptrKey);

	if (keyLength < hsec->fileLength) {
		fprintf(stderr, "Keystream file must be at least %u bytes long\n", hsec->fileLength);
		fclose(hsec->fptrKey);
		return -1;
	}

	printf("Data buffer prior to XOR encrypt:\n");
	hexDump(hsec->data, hsec->encryptionBufferLength);

	for (i = sizeof(CLOAK_HEADER);i < hsec->encryptionBufferLength;i++) {
		ch = fgetc(hsec->fptrKey);

		if (ch == EOF) {
			fprintf(stderr, "Got EOF from keystream file %s\n", pszKeystreamFilename);
			fclose(hsec->fptrKey);
			return -1;
		}

		hsec->data[i] = (hsec->data[i] ^ (uint8_t)ch);
	}

	printf("Data buffer after XOR encrypt:\n");
	hexDump(hsec->data, hsec->encryptionBufferLength);

	fclose(hsec->fptrKey);

	return 0;
}

void rdr_close(HSECRW hsec)
{
	if (hsec->fptrSecret != NULL) {
		fclose(hsec->fptrSecret);
	}

	if (hsec->fptrKey != NULL) {
		fclose(hsec->fptrKey);
	}

	//free(hsec->data);
	free(hsec);
}

uint32_t rdr_get_block_size(HSECRW hsec)
{
	return hsec->blockSize;
}

uint32_t rdr_get_data_length(HSECRW hsec)
{
	return hsec->dataFrameLength;
}

uint32_t rdr_get_file_length(HSECRW hsec)
{
	return hsec->fileLength;
}

boolean rdr_has_more_blocks(HSECRW hsec)
{
	boolean hasMore;

	if (hsec->algo == aes256) {
		hasMore = (hsec->counter < hsec->dataFrameLength) ? true : false;
	}
	else if (hsec->algo == xor) {
		hasMore = (hsec->counter < hsec->fileLength) ? true : false;
	}
	else {
		hasMore = (hsec->counter < hsec->fileLength) ? true : false;
	}

	return hasMore;
}

uint32_t rdr_read_encrypted_block(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength)
{
	uint32_t			bytesRead;

	if (bufferLength < hsec->blockSize) {
		fprintf(stderr, "Buffer length must be as least as big as the block size: %u\n", hsec->blockSize);
		return 0;
	}

	memcpy(buffer, random_block, hsec->blockSize);

	if (hsec->algo == aes256) {
		bytesRead = ((hsec->dataFrameLength - hsec->counter) >= hsec->blockSize ? hsec->blockSize : (hsec->dataFrameLength - hsec->counter));
		
		memcpy(buffer, &hsec->data[hsec->counter], bytesRead);
	}
	else if (hsec->algo == xor) {
		bytesRead = ((hsec->fileLength - hsec->counter) >= hsec->blockSize ? hsec->blockSize : (hsec->fileLength - hsec->counter));
		memcpy(buffer, &hsec->data[hsec->counter], bytesRead);
	}
	else {
		bytesRead = fread(buffer, 1, hsec->blockSize, hsec->fptrSecret);
	}

	hsec->blockCounter++;
	hsec->counter += bytesRead;

	return bytesRead;
}

HSECRW wrtr_open(char * pszFilename, encryption_algo a)
{
	HSECRW			hsec;

	hsec = (HSECRW)malloc(sizeof(struct _secret_rw_handle));

	if (hsec == NULL) {
		fprintf(stderr, "Failed to allocate memory for cloak handle\n");
		return NULL;
	}

	hsec->blockSize = BLOCK_SIZE;
	hsec->algo = a;
	hsec->counter = 0;
	hsec->blockCounter = 0;

	hsec->fptrKey = NULL;

	hsec->fptrSecret = fopen(pszFilename, "wb");

	if (hsec->fptrSecret == NULL) {
		fprintf(stderr, "Failed to open file writer with file %s: %s\n", pszFilename, strerror(errno));
		return NULL;
	}

	return hsec;
}

void wrtr_close(HSECRW hsec)
{
	if (hsec->fptrSecret != NULL) {
		fclose(hsec->fptrSecret);
	}

	if (hsec->fptrKey != NULL) {
		fclose(hsec->fptrKey);
	}

	if (hsec->data != NULL) {
		free(hsec->data);
	}

	free(hsec);
}

uint32_t wrtr_get_block_size(HSECRW hsec)
{
	return hsec->blockSize;
}

boolean wrtr_has_more_blocks(HSECRW hsec)
{
	return (hsec->counter < hsec->encryptionBufferLength) ? true : false;
}

int wrtr_set_keystream_file(HSECRW hsec, char * pszFilename)
{
	uint32_t		keyLength;

	hsec->fptrKey = fopen(pszFilename, "rb");

	if (hsec->fptrKey == NULL) {
		fprintf(stderr, "Failed to open keystream file %s: %s\n", pszFilename, strerror(errno));
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

int wrtr_set_key_aes(HSECRW hsec, uint8_t * key, uint32_t keyLength)
{
	if (hsec->algo == aes256) {
		int			err;

		err = gcry_cipher_open(
							&hsec->cipherHandle,
							GCRY_CIPHER_RIJNDAEL256,
							GCRY_CIPHER_MODE_CBC,
							0);

		if (err) {
			fprintf(stderr, "Failed to open cipher with gcrypt\n");
			fclose(hsec->fptrSecret);
			free(hsec);
			return -1;
		}

		err = gcry_cipher_setkey(
							hsec->cipherHandle,
							(const void *)key,
							keyLength);

		if (err) {
			fprintf(stderr, "Failed to set key with gcrypt: %s\n", gcry_strerror(err));
			fclose(hsec->fptrSecret);
			free(hsec);
			return -1;
		}
	}

	return 0;
}

int wrtr_write_decrypted_block(HSECRW hsec, uint8_t * buffer, uint32_t bufferLength)
{
	CLOAK_HEADER		header;
	int					i;
	int					err;
	uint32_t			blklen;

	if (bufferLength < hsec->blockSize) {
		fprintf(stderr, "Buffer must be at least 1 block long: %u bytes", hsec->blockSize);
		return -1;
	}

	if (hsec->blockCounter == 0) {
		if (hsec->algo == aes256 || hsec->algo == xor) {
			int bufferIndex = 0;

			/*
			** XOR the header with random data...
			*/
			xorBuffer(buffer, &random_block[2048], sizeof(CLOAK_HEADER));
			memcpy(&header, buffer, sizeof(CLOAK_HEADER));

			bufferIndex = sizeof(CLOAK_HEADER);

			hsec->fileLength = header.fileLength;
			hsec->encryptionBufferLength = header.encryptionBufferLength;
			hsec->dataFrameLength  = header.dataFrameLength;

			hsec->numBlocks = hsec->encryptionBufferLength / hsec->blockSize;

			if (hsec->algo == aes256) {
				uint8_t *		iv;

				hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

				if (hsec->data == NULL) {
					fprintf(stderr, "Failed to allocate %u bytes for data buffer\n", hsec->dataFrameLength);
					return -1;
				}

				blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

				iv = (uint8_t *)malloc(blklen);

				if (iv == NULL) {
					fprintf(stderr, "Failed to allocate memory for IV block\n");
					free(hsec->data);
					return -1;
				}

				memcpy(iv, &buffer[bufferIndex], blklen);
				bufferIndex += blklen;

				err = gcry_cipher_setiv(
									hsec->cipherHandle,
									iv,
									blklen);

				if (err) {
					fprintf(stderr, "Failed to set IV with gcrypt\n");
					free(iv);
					free(hsec->data);
					fclose(hsec->fptrSecret);
					free(hsec);
					return -1;
				}

				free(iv);

				memcpy(hsec->data, &buffer[bufferIndex], (bufferLength - bufferIndex));
			}
			else if (hsec->algo == xor) {
				hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

				if (hsec->data == NULL) {
					fprintf(stderr, "Failed to allocate %u bytes for data buffer\n", hsec->dataFrameLength);
					return -1;
				}

				memcpy(hsec->data, &buffer[bufferIndex], (bufferLength - bufferIndex));
			}
				
			hsec->counter += bufferIndex;
		}
		else {
			fwrite(buffer, 1, bufferLength, hsec->fptrSecret);
		}
	}
	else if (hsec->blockCounter < hsec->numBlocks) {
		if (hsec->algo == aes256) {
			memcpy(&hsec->data[hsec->counter], buffer, bufferLength);
		}
		else if (hsec->algo == xor) {
			memcpy(&hsec->data[hsec->counter], buffer, bufferLength);
		}
		else {
			fwrite(buffer, 1, bufferLength, hsec->fptrSecret);
		}

		hsec->counter += bufferLength;
	}
	else if (hsec->blockCounter == hsec->numBlocks) {
		if (hsec->algo == aes256) {
			memcpy(&hsec->data[hsec->counter], buffer, (hsec->encryptionBufferLength - hsec->counter));

			err = gcry_cipher_decrypt(
									hsec->cipherHandle,
									hsec->data,
									hsec->encryptionBufferLength,
									NULL,
									0);

			if (err) {
				fprintf(stderr, "Failed to decrypt buffer: %s", gcry_strerror(err));
				free(hsec);
				return -1;
			}

			gcry_cipher_close(hsec->cipherHandle);

			fwrite(hsec->data, 1, hsec->fileLength, hsec->fptrSecret);
		}
		else if (hsec->algo == xor) {
			memcpy(&hsec->data[hsec->counter], buffer, (hsec->encryptionBufferLength - hsec->counter));

			for (i = 0;i < (hsec->encryptionBufferLength - hsec->counter);i++) {
				hsec->data[i] = hsec->data[i] ^ (uint8_t)fgetc(hsec->fptrKey);
			}

			fwrite(hsec->data, 1, hsec->fileLength, hsec->fptrSecret);
		}
		else {
			fwrite(buffer, 1, bufferLength, hsec->fptrSecret);
		}

		return 1;
	}
	else {
		return 1;
	}
		
	hsec->blockCounter++;

	return 0;
}

