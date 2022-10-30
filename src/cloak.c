#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "cloak_types.h"
#include "secretrw.h"
#include "imgrw.h"
#include "utils.h"

#define MAX_PASSWORD_LENGTH						255
#define MEMID_IMAGEDATA							0x0001


typedef enum {
	quality_high = 1,
	quality_medium = 2,
	quality_low = 4,

	/*
	** Testing only!
	*/
	quality_none = 8
}
merge_quality;



uint8_t getBitMask(merge_quality quality)
{
	uint8_t mask = 0x00;

	switch (quality) {
		case quality_high:
			mask = 0x01;
			break;

		case quality_medium:
			mask = 0x03;
			break;

		case quality_low:
			mask = 0x0F;
			break;

		case quality_none:
			mask = 0xFF;
			break;
	}

	return mask;
}

int getNumImageBytesRequired(merge_quality quality)
{
	return (8 / quality);
}

void mergeSecretByte(uint8_t * imageBytes, int numImageBytes, uint8_t secretByte, merge_quality quality)
{
	uint8_t			mask;
	uint8_t			secretBits;
	int				i;
	int				bitCounter = 0;

	mask = getBitMask(quality);

    for (i = 0;i < numImageBytes;i++) {
        secretBits = (secretByte >> bitCounter) & mask;
        imageBytes[i] = (imageBytes[i] & ~mask) | secretBits;

        bitCounter += quality;

        if (bitCounter == 8) {
            bitCounter = 0;
        }
    }
}

uint8_t extractSecretByte(uint8_t * imageBytes, uint32_t numImageBytes, merge_quality quality)
{
	uint8_t			mask;
	uint8_t			secretBits = 0x00;
	uint8_t			secretByte = 0x00;
	int				i;
	int				bitCounter = 0;

	mask = getBitMask(quality);

    for (i = 0;i < numImageBytes;i++) {
        secretBits = imageBytes[i] & mask;
        secretByte += secretBits << bitCounter;

        bitCounter += quality;

        if (bitCounter == 8) {
            bitCounter = 0;
        }
    }

	return secretByte;
}

uint32_t getImageCapacity(char * pszInputImageFile, merge_quality quality)
{
	HIMG			himgRead;
	uint32_t		imageDataLen;
	uint32_t		imageCapacity;
	int				numImgBytesRequired = 0;

	himgRead = imgrdr_open(pszInputImageFile);

	if (himgRead == NULL) {
		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputImageFile, strerror(errno));
		exit(-1);
	}

	imageDataLen = imgrdr_get_data_length(himgRead);
	
	numImgBytesRequired = getNumImageBytesRequired(quality);
	
	imageCapacity = imageDataLen / numImgBytesRequired;

	imgrdr_close(himgRead);
	imgrdr_destroy_handle(himgRead);

	return imageCapacity;
}

int merge(
		char * pszInputImageFile, 
		char * pszSecretFile, 
		char * pszKeystreamFile,
		char * pszOutputImageFile,
		merge_quality quality, 
		encryption_algo algo, 
		uint8_t * key, 
		uint32_t keyLength)
{
	HSECRW			hsec;
	HIMG			himgRead;
	HIMG			himgWrite;
	uint8_t 		secretDataBlock[SECRETRW_BLOCK_SIZE];
	uint8_t *		imageData;
	uint8_t			secretByte = 0x00;
	uint32_t		secretDataBlockLen;
	uint32_t		imageDataLen;
	uint32_t		secretBytesRemaining = 0;
	uint32_t		imageBytesRead;
	uint32_t		imageDataIndex = 0U;
    uint32_t        requiredImageLength;
	int				numImgBytesRequired = 0;
	int				secretBufferIndex = 0;
	int				rtn;
	img_type		imageType;

	hsec = rdr_open(pszSecretFile, algo);

	if (hsec == NULL) {
		fprintf(stderr, "Could not open input file %s: %s\n", pszSecretFile, strerror(errno));
		exit(-1);
	}
	
	if (algo == aes256) {
		rtn = rdr_encrypt_aes256(hsec, key, keyLength);

		if (rtn) {
			exit(-1);
		}
	}
	else if (algo == xor) {
		rtn = rdr_encrypt_xor(hsec, pszKeystreamFile);

		if (rtn) {
			exit(-1);
		}
	}

	secretDataBlockLen = rdr_get_block_size(hsec);
	
	himgRead = imgrdr_open(pszInputImageFile);

	if (himgRead == NULL) {
		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputImageFile, strerror(errno));
		exit(-1);
	}

	imageDataLen = imgrdr_get_data_length(himgRead);
	
	numImgBytesRequired = getNumImageBytesRequired(quality);
	
	requiredImageLength = 
		rdr_get_data_length(hsec) * 
		getNumImageBytesRequired(quality);
	
	/*
	** Check the image capacity, will our file fit...?
	*/
	if (imageDataLen < requiredImageLength) {
		fprintf(
			stderr, 
			"The image %s is not large enough to store the file %s\n", 
			pszInputImageFile, 
			pszSecretFile);
		fprintf(
			stderr, 
			"The file %s requires %u of image data, image %s has a max capacity of %u bytes.\n", 
			pszSecretFile, 
			rdr_get_data_length(hsec), 
			pszInputImageFile, 
			(imageDataLen / numImgBytesRequired));
		fprintf(
			stderr, 
			"Consider compressing the file, or using a lower quality setting.\n");

		rdr_close(hsec);
		imgrdr_close(himgRead);
		exit(-1);
	}

	imageData = (uint8_t *)malloc(imageDataLen);

	if (imageData == NULL) {
		fprintf(stderr, "Could not allocate memory for image data\n");
		rdr_close(hsec);
		imgrdr_close(himgRead);
		exit(-1);
	}

	imageBytesRead = imgrdr_read(himgRead, imageData, imageDataLen);

	/*
	** Only check if we have enough image bytes for PNG images,
	** this check fails for BMP images, and I haven't worked out
	** why yet...
	*/
	if (imgrdr_get_type(himgRead) == img_png) {
		if (imageBytesRead < imageDataLen) {
			fprintf(stderr, "Expected %u bytes of image data, but got %u bytes\n", imageDataLen, imageBytesRead);
			rdr_close(hsec);
			imgrdr_close(himgRead);
			exit(-1);
		}
	}

	while (rdr_has_more_blocks(hsec)) {
		secretBytesRemaining = rdr_read_encrypted_block(hsec, secretDataBlock, secretDataBlockLen);

		for(secretBufferIndex = 0;secretBufferIndex < secretBytesRemaining;secretBufferIndex++) {
			secretByte = secretDataBlock[secretBufferIndex];

			mergeSecretByte(
					&imageData[imageDataIndex], 
					numImgBytesRequired, 
					secretByte, 
					quality);

			imageDataIndex += (uint32_t)numImgBytesRequired;
		}
	}

	imageType = imgrdr_get_type(himgRead);

	himgWrite = imgwrtr_open(pszOutputImageFile, imageType);

	imgrdr_copy_header(himgWrite, himgRead);

	imgrdr_close(himgRead);
	imgrdr_destroy_handle(himgRead);

	imgwrtr_write_header(himgWrite);
	imgwrtr_write(himgWrite, imageData, imageDataLen);
	imgwrtr_close(himgWrite);

	imgrdr_destroy_handle(himgWrite);

	free(imageData);

	rdr_close(hsec);

	return 0;
}

int extract(
		char * pszInputImageFile, 
		char * pszKeystreamFile,
		char * pszSecretFile, 
		merge_quality quality, 
		encryption_algo algo, 
		uint8_t * key, 
		uint32_t keyLength)
{
	HSECRW			hsec;
	HIMG			himgRead;
	uint8_t 		secretDataBlock[SECRETRW_BLOCK_SIZE];
	uint8_t *		imageData;
	uint8_t			secretByte = 0x00;
	uint32_t		secretDataBlockLen;
	uint32_t		imageDataLen;
	uint32_t		imageBytesRead;
	uint32_t		imageDataIndex = 0U;
	int				numImgBytesRequired = 0;
	int				secretBufferIndex = 0;
	int				rtn;

	himgRead = imgrdr_open(pszInputImageFile);

	if (himgRead == NULL) {
		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputImageFile, strerror(errno));
		exit(-1);
	}

	imageDataLen = imgrdr_get_data_length(himgRead);

	imageData = (uint8_t *)malloc(imageDataLen);

	if (imageData == NULL) {
		fprintf(stderr, "Could not allocate memory for image data\n");
		imgrdr_close(himgRead);
		exit(-1);
	}

	imageBytesRead = imgrdr_read(himgRead, imageData, imageDataLen);

	/*
	** Only check if we have enough image bytes for PNG images,
	** this check fails for BMP images, and I haven't worked out
	** why yet...
	*/
	if (imgrdr_get_type(himgRead) == img_png) {
		if (imageBytesRead < imageDataLen) {
			fprintf(stderr, "Expected %u bytes of image data, but got %u bytes\n", imageDataLen, imageBytesRead);
			imgrdr_close(himgRead);
			exit(-1);
		}
	}

	imgrdr_close(himgRead);

	imgrdr_destroy_handle(himgRead);

	hsec = wrtr_open(pszSecretFile, algo);

	if (hsec == NULL) {
		fprintf(stderr, "Failed to open output file %s\n", pszSecretFile);
		free(imageData);
		exit(-1);
	}

	secretDataBlockLen = wrtr_get_block_size(hsec);

	if (algo == aes256) {
		if (wrtr_set_key_aes(hsec, key, keyLength)) {
			fprintf(stderr, "Failed to set AES key\n");
			free(imageData);
			wrtr_close(hsec);
			exit(-1);
		}
	}
	else if (algo == xor) {
		wrtr_set_keystream_file(hsec, pszKeystreamFile);
	}

	numImgBytesRequired = getNumImageBytesRequired(quality);

	secretBufferIndex = 0;

	for (
		imageDataIndex = 0;
		imageDataIndex < imageDataLen;
		imageDataIndex += numImgBytesRequired)
	{
		secretByte = extractSecretByte(&imageData[imageDataIndex], numImgBytesRequired, quality);

		secretDataBlock[secretBufferIndex++] = secretByte;

		if (secretBufferIndex == secretDataBlockLen) {
			rtn = wrtr_write_decrypted_block(hsec, secretDataBlock, secretDataBlockLen);

			if (rtn < 0) {
				fprintf(stderr, "Error writing secret block\n");
				free(imageData);
				wrtr_close(hsec);

				exit(-1);
			}
			else if (rtn > 0) {
				/*
				** We've finished...
				*/
				break;
			}

			secretBufferIndex = 0;
		}
	}

	wrtr_close(hsec);

	free(imageData);

	return 0;
}
