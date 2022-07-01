#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <gcrypt.h>

#include "cloak_types.h"
#include "secretrw.h"
#include "pngrw.h"
#include "random_block.h"
#include "utils.h"

#define MAX_PASSWORD_LENGTH						255

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


void printUsage()
{
    printf("Usage:\n");
    printf("    clk --help (show this help)\n");
    printf("    clk [options] source-image\n");
    printf("    options: -o [output file]\n");
    printf("             -f [file to cloak]\n");
    printf("             -k [keystream file for one-time pad encryption]\n");
    printf("             -q [merge quality] either 1, 2, or 4 bits per byte\n\n");
}

uint8_t getBitMask(merge_quality quality)
{
	uint8_t mask = 0x00;

	for (int i = 0;i < quality;i++) {
		mask += (0x01 << i) & 0xFF;
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
	int				pos = 0;

	mask = getBitMask(quality);

    for (i = 0;i < numImageBytes;i++) {
        secretBits = (secretByte >> bitCounter) & mask;
        imageBytes[i] = (imageBytes[i] & ~mask) | secretBits;

        bitCounter += quality;

        if (bitCounter == 8) {
            bitCounter = 0;
            pos++;
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

uint32_t getKey(uint8_t * keyBuffer, uint32_t keyBufferLength)
{
	static char		szPassword[MAX_PASSWORD_LENGTH + 1];
	int				i = 0;
	int				ch = 0;
	uint32_t		keySize;

	printf("Enter password: ");
	
	while (i < MAX_PASSWORD_LENGTH) {
		ch = __getch();

		if (ch != '\n' && ch != '\r') {
			putchar('*');
			fflush(stdout);
			szPassword[i++] = (char)ch;
		}
		else {
			break;
		}
	}

	putchar('\n');
	fflush(stdout);
	szPassword[i] = 0;

	gcry_md_hash_buffer(GCRY_MD_SHA3_256, keyBuffer, szPassword, i);

	keySize = gcry_md_get_algo_dlen(GCRY_MD_SHA3_256);

	return keySize;
}

int main(int argc, char ** argv)
{
	int				i;
	char *			arg;
	char *			pszExtension;
	char *			pszInputFilename = NULL;
	char *			pszKeystreamFilename = NULL;
	char *			pszOutputFilename = NULL;
	char *			pszSourceFilename = NULL;
	const uint32_t	keyBufferLen = 64U;
	uint8_t *		key = NULL;
	uint32_t		keyLength;
	boolean			isMerge = false;
	merge_quality	quality;
	encryption_algo	algo;
	
    if (argc > 1) {
        for (i = 1;i < argc;i++) {
            arg = argv[i];

            if (arg[0] == '-') {
                if (strncmp(arg, "--help", 6) == 0) {
                    printUsage();
                    return 0;
                }
                else if (strncmp(arg, "-f", 2) == 0) {
                    pszInputFilename = strdup(argv[i + 1]);
                }
                else if (strncmp(arg, "-k", 2) == 0) {
                    pszKeystreamFilename = strdup(argv[i + 1]);
                }
                else if (strncmp(arg, "-o", 2) == 0) {
                    pszOutputFilename = strdup(argv[i + 1]);
                }
                else if (strncmp(arg, "-q", 2) == 0) {
                    int q = atoi(argv[i + 1]);

                    switch (q) {
                        case quality_high:
                        case quality_medium:
                        case quality_low:
						case quality_none:
							quality = q;
                            break;

                        default:
                            printf("Invalid quality supplied, valid quality values are 1, 2, or 4 bits...\n");
                            return -1;
                    }
                }
                else {
                    printf("Invalid option %s - %s --help for help", arg, argv[0]);
                    return -1;
                }
            }
        }
    }
    else {
        printUsage();
        return -1;
    }

	pszSourceFilename = strdup(argv[argc - 1]);
	
	if (pszInputFilename != NULL) {
		isMerge = true;
	}
	
    pszExtension = getFileExtension(pszSourceFilename);
    
    if (pszExtension != NULL) {
    	if (strcmp(pszExtension, "png") == 0) {
    		printf("Processing PNG image file %s\n", pszSourceFilename);
    	}
//     	else if (strcmp(pszExtension, "bmp") == 0) {
//     		printf("Processing BMP image file %s\n", pszSourceFilename);
//     	}
//     	else if (strcmp(pszExtension, "wav") == 0) {
//     		printf("Processing WAV sound file %s\n", pszSourceFilename);
//     	}
    	else {
    		fprintf(stderr, "Unsupported file extension %s\n", pszExtension);
    		exit(-1);
    	}
    }
    else {
		fprintf(stderr, "File extension not found for file %s\n", pszSourceFilename);
		exit(-1);
    }

	if (pszKeystreamFilename != NULL) {
		algo = xor;
	}
	else {
		algo = aes256;
    
		key = (uint8_t *)malloc(keyBufferLen);

		if (key == NULL) {
			fprintf(stderr, "Failed to allocate memory for key\n");
			exit(-1);
		}

		keyLength = getKey(key, keyBufferLen);
	}

	HSECRW			hsec;
	HPNG			hpng;
	uint8_t *		secretDataBlock;
	uint8_t *		imageData;
	uint8_t			secretByte = 0x00;
	uint32_t		secretDataBlockLen;
	uint32_t		imageDataLen;
	uint32_t		secretBytesRemaining = 0;
	uint32_t		imageBytesRead;
	uint32_t		imageDataIndex = 0U;
	int				numImgBytesRequired = 0;
	int				secretBufferIndex = 0;
	int				rtn;

    if (isMerge) {
		hsec = rdr_open(pszInputFilename, algo);

    	if (hsec == NULL) {
    		fprintf(stderr, "Could not open input file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
    	}
    	
		if (algo == aes256) {
			rdr_encrypt_aes256(hsec, key, keyLength);
		}
		else if (algo == xor) {
			rdr_set_keystream_file(hsec, pszKeystreamFilename);
		}

		secretDataBlockLen = rdr_get_block_size(hsec);

    	secretDataBlock = (uint8_t *)malloc(secretDataBlockLen);
    	
    	if (secretDataBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for input block\n");
			rdr_close(hsec);
			exit(-1);
    	}
    	
		hpng = pngrw_open(pszSourceFilename, pszOutputFilename);

		if (hpng == NULL) {
    		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
		}

		imageDataLen = pngrw_get_data_length(hpng);

		imageData = (uint8_t *)malloc(imageDataLen);

		if (imageData == NULL) {
    		fprintf(stderr, "Could not allocate memory for image data\n");
			rdr_close(hsec);
			pngrw_close(hpng);
			exit(-1);
		}

		imageBytesRead = pngrw_read(hpng, imageData, imageDataLen);

		if (imageBytesRead < imageDataLen) {
			fprintf(stderr, "Expected %u bytes of image data, but got %u bytes\n", imageDataLen, imageBytesRead);
			rdr_close(hsec);
			pngrw_close(hpng);
			exit(-1);
		}

		numImgBytesRequired = getNumImageBytesRequired(quality);

		while (rdr_has_more_blocks(hsec)) {
			secretBytesRemaining = rdr_read_encrypted_block(hsec, secretDataBlock, secretDataBlockLen);
			secretBufferIndex = 0;

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

		pngrw_write(hpng, imageData, imageDataLen);

		free(secretDataBlock);
		free(imageData);
    	
    	rdr_close(hsec);
		pngrw_close(hpng);
    }
    else {
		/*
		** Extract our secret file from the source image...
		*/
		hpng = pngrw_open(pszSourceFilename, NULL);

		if (hpng == NULL) {
    		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
		}

		imageDataLen = pngrw_get_data_length(hpng);

		imageData = (uint8_t *)malloc(imageDataLen);

		if (imageData == NULL) {
    		fprintf(stderr, "Could not allocate memory for image data\n");
			pngrw_close(hpng);
			exit(-1);
		}

		imageBytesRead = pngrw_read(hpng, imageData, imageDataLen);

		hsec = wrtr_open(pszOutputFilename, algo);

		if (hsec == NULL) {
			fprintf(stderr, "Failed to open output file %s\n", pszOutputFilename);
			free(imageData);
			exit(-1);
		}

		secretDataBlockLen = wrtr_get_block_size(hsec);
		secretDataBlock = (uint8_t *)malloc(secretDataBlockLen);

		if (secretDataBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for secret data block\n");
			free(imageData);
			pngrw_close(hpng);
			exit(-1);
		}

		if (algo == aes256) {
			if (wrtr_set_key_aes(hsec, key, keyLength)) {
				fprintf(stderr, "Failed to set AES key\n");
				free(imageData);
				pngrw_close(hpng);
				wrtr_close(hsec);
				exit(-1);
			}
		}
		else if (algo == xor) {
			wrtr_set_keystream_file(hsec, pszKeystreamFilename);
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
					free(secretDataBlock);
					free(imageData);
					pngrw_close(hpng);
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

		free(imageData);
		free(secretDataBlock);

		pngrw_close(hpng);
		wrtr_close(hsec);
    }

	secureFree(key, keyBufferLen);

	return 0;
}
