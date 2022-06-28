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
		int				index = 0;
		uint32_t		blklen;
		uint32_t		bytesRead;
		uint8_t *		iv;
		CLOAK_HEADER	header;

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
		xorBuffer(&header, &random_block[2048], sizeof(CLOAK_HEADER));

		memcpy(&hsec->data[index], &header, sizeof(CLOAK_HEADER));
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
		hsec->encryptionBufferLength = hsec->fileLength + (hsec->blockSize - (hsec->fileLength % hsec->blockSize));
		hsec->dataFrameLength = hsec->encryptionBufferLength + sizeof(CLOAK_HEADER);
	}
	else {
		hsec->encryptionBufferLength = hsec->fileLength + (hsec->blockSize - (hsec->fileLength % hsec->blockSize));
		hsec->dataFrameLength = hsec->encryptionBufferLength;
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

int rdr_set_keystream_file(HSECRW hsec, char * pszKeystreamFilename)
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
	return (hsec->counter < hsec->dataFrameLength) ? true : false;
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
		uint32_t		index = 0;

		if (hsec->blockCounter == 0) {
			CLOAK_HEADER	header;

			header.fileLength = hsec->fileLength;
			header.dataFrameLength = hsec->dataFrameLength;
			header.encryptionBufferLength = hsec->encryptionBufferLength;

			/*
			** XOR the header with random data...
			*/
			xorBuffer(&header, &random_block[2048], sizeof(CLOAK_HEADER));

			memcpy(buffer, &header, sizeof(CLOAK_HEADER));

			bytesRead = sizeof(CLOAK_HEADER);
			bytesRead += fread(&buffer[sizeof(CLOAK_HEADER)], 1, (hsec->blockSize - sizeof(CLOAK_HEADER)), hsec->fptrSecret);
		}
		else {
			bytesRead = fread(buffer, 1, hsec->blockSize, hsec->fptrSecret);
		}

		while (index < hsec->blockSize) {
			buffer[index] = buffer[index] ^ (uint8_t)fgetc(hsec->fptrKey);
			index++;
		}
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

uint32_t wrtr_get_block_size(HSECRW hsec)
{
	return hsec->blockSize;
}

boolean wrtr_has_more_blocks(HSECRW hsec)
{
	return (hsec->counter < hsec->dataFrameLength) ? true : false;
}

int wrtr_set_keystream_file(HSECRW hsec, char * pszFilename)
{
	return rdr_set_keystream_file(hsec, pszFilename);
}

int wrtr_set_key_aes(HSECRW hsec, uint8_t * key, uint32_t keyLength)
{
	if (hsec->algo == aes256) {
		int			err;

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
			memcpy(&header, buffer, sizeof(CLOAK_HEADER));

			/*
			** XOR the header with random data...
			*/
			xorBuffer(&header, &random_block[2048], sizeof(CLOAK_HEADER));

			hsec->fileLength = header.fileLength;
			hsec->encryptionBufferLength = header.encryptionBufferLength;
			hsec->dataFrameLength  = header.dataFrameLength;

			hsec->counter = sizeof(CLOAK_HEADER);

			if (hsec->algo == aes256) {
				uint8_t *		iv;

				hsec->data = (uint8_t *)malloc(hsec->dataFrameLength);

				if (hsec == NULL) {
					fprintf(stderr, "Failed to allocate %u bytes for data buffer\n", hsec->dataFrameLength);
					return -1;
				}

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

				blklen = gcry_cipher_get_algo_blklen(GCRY_CIPHER_RIJNDAEL256);

				iv = (uint8_t *)malloc(blklen);

				if (iv == NULL) {
					fprintf(stderr, "Failed to allocate memory for IV block\n");
					free(hsec->data);
					return -1;
				}

				memcpy(iv, buffer, blklen);

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

				/*
				** We've set the IV here, so we're not going to write
				** it to the data buffer, so take account of that...
				*/
				hsec->dataFrameLength -= blklen;

				memcpy(hsec->data, &buffer[hsec->counter], (bufferLength - hsec->counter));
				hsec->counter += (bufferLength - hsec->counter);
			}
			else if (hsec->algo == xor) {
				for (i = hsec->counter;i < (bufferLength - hsec->counter);i++) {
					buffer[i] = buffer[i] ^ fgetc(hsec->fptrKey);
					fputc(buffer[i], hsec->fptrSecret);
					hsec->counter += (bufferLength - hsec->counter);
				}
			}
		}
		else {
			fwrite(buffer, 1, bufferLength, hsec->fptrSecret);
			hsec->counter += bufferLength;
		}
	}
	else {
		if ((hsec->counter + bufferLength) > hsec->dataFrameLength) {
			bufferLength = (hsec->dataFrameLength - hsec->counter);
		}

		if (hsec->algo == aes256) {
			memcpy(&hsec->data[hsec->counter], buffer, bufferLength);

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
			for (i = 0;i < bufferLength;i++) {
				buffer[i] = buffer[i] ^ fgetc(hsec->fptrKey);
				fputc(buffer[i], hsec->fptrSecret);
			}
		}
		else {
			fwrite(buffer, 1, bufferLength, hsec->fptrSecret);
		}
			
		hsec->counter += bufferLength;
	}

	hsec->blockCounter++;

	return 0;
}

