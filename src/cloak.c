#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <gcrypt.h>

#include "cloak_types.h"
#include "secretrw.h"
#include "imgrw.h"
#include "random_block.h"
#include "utils.h"
#include "version.h"

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


int _getProgNameStartPos(char * pszProgName)
{
	int i = strlen(pszProgName);

	while (pszProgName[i] != '/' && i > 0) {
		i--;	
	}

	if (pszProgName[i] == '/') {
		i++;
	}

	return i;
}

void printVersion(char * pszProgName)
{
    printf(
		"%s version %s built: %s\n\n", 
		&pszProgName[_getProgNameStartPos(pszProgName)], 
		getVersion(), 
		getBuildDate());
}

void printUsage(char * pszProgName)
{
	printVersion(pszProgName);

	printf("Using %s:\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
    printf("    %s --help (show this help)\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
    printf("    %s [options] source-image\n", &pszProgName[_getProgNameStartPos(pszProgName)]);
    printf("    options: -o [output file]\n");
    printf("             -f [input file to cloak]\n");
    printf("             -k [keystream file for one-time pad encryption]\n");
    printf("             --merge-quality=value where value is:\n");
	printf("                       'high', 'medium', or 'low'\n");
    printf("             --algo=value where value is:\n");
	printf("                    'aes' for AES-256 encryption (prompt for password),\n");
	printf("                    'xor' for one-time pad encryption (-k is mandatory),\n");
	printf("                    'none' for no encryption (hide only)\n\n");
}

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
	char *			pszAlgorithm;
	char *			pszQuality;
	const uint32_t	keyBufferLen = 64U;
	uint8_t *		key = NULL;
	uint32_t		keyLength;
	boolean			isMerge = false;
	merge_quality	quality = quality_high;
	encryption_algo	algo = none;
	
    if (argc > 1) {
        for (i = 1;i < argc;i++) {
            arg = argv[i];

            if (arg[0] == '-') {
                if (strncmp(arg, "--help", 6) == 0) {
                    printUsage(argv[0]);
                    return 0;
                }
                else if (strncmp(arg, "--version", 9) == 0) {
					printVersion(argv[0]);
					return 0;
                }
                else if (strncmp(arg, "--algo=", 7) == 0) {
                    pszAlgorithm = strdup(&arg[7]);

					if (strncmp(pszAlgorithm, "aes", 3) == 0) {
						algo = aes256;
					}
					else if (strncmp(pszAlgorithm, "xor", 3) == 0) {
						algo = xor;
					}
					else if (strncmp(pszAlgorithm, "none", 4) == 0) {
						algo = none;
					}
					else {
						printf("Unrecognised encryption algorithm '%s'\n", pszAlgorithm);
                    	printUsage(argv[0]);
						dbg_free(pszAlgorithm, __FILE__, __LINE__);
						return -1;
					}

					dbg_free(pszAlgorithm, __FILE__, __LINE__);
                }
                else if (strncmp(arg, "--merge-quality=", 16) == 0) {
                    pszQuality = strdup(&arg[16]);

					if (strncmp(pszQuality, "high", 4) == 0) {
						quality = quality_high;
					}
					else if (strncmp(pszQuality, "medium", 6) == 0) {
						quality = quality_medium;
					}
					else if (strncmp(pszQuality, "low", 3) == 0) {
						quality = quality_low;
					}
					else if (strncmp(pszQuality, "none", 4) == 0) {
						quality = quality_none;
					}
					else {
						printf("Unrecognised merge quality '%s'\n", pszQuality);
                    	printUsage(argv[0]);
						dbg_free(pszQuality, __FILE__, __LINE__);
						return -1;
					}

					dbg_free(pszQuality, __FILE__, __LINE__);
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
                else {
                    printf("Invalid option %s - %s --help for help", arg, argv[0]);
                    return -1;
                }
            }
        }
    }
    else {
    	printUsage(argv[0]);
        return -1;
    }

	if (algo == xor && pszKeystreamFilename == NULL) {
		printf("For encryption algorithm 'xor', you must specify a key stream file.\n");
		exit(-1);
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

	if (algo == aes256) {
		key = (uint8_t *)malloc(keyBufferLen);

		if (key == NULL) {
			fprintf(stderr, "Failed to allocate memory for key\n");
			exit(-1);
		}

		keyLength = getKey(key, keyBufferLen);
	}

	HSECRW			hsec;
	HIMG			himg;
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
			rtn = rdr_encrypt_aes256(hsec, key, keyLength);

			if (rtn) {
				exit(-1);
			}
		}
		else if (algo == xor) {
			rtn = rdr_encrypt_xor(hsec, pszKeystreamFilename);

			if (rtn) {
				exit(-1);
			}
		}

		secretDataBlockLen = rdr_get_block_size(hsec);

    	secretDataBlock = (uint8_t *)malloc(secretDataBlockLen);
    	
    	if (secretDataBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for input block\n");
			rdr_close(hsec);
			exit(-1);
    	}
    	
		himg = pngrdr_open(pszSourceFilename);

		if (himg == NULL) {
    		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
		}

		imageDataLen = pngrdr_get_data_length(himg);
		
		/*
		** Check the image capacity, will our file fit...?
		*/
		if (imageDataLen < (rdr_get_data_length(hsec) * getNumImageBytesRequired(quality))) {
			fprintf(
				stderr, 
				"The image %s is not large enough to store the file %s\n", 
				pszSourceFilename, 
				pszInputFilename);
			fprintf(
				stderr, 
				"The file %s requires %u of image data, image %s has a maximum capacity of %u bytes\n", 
				pszInputFilename, 
				rdr_get_data_length(hsec), 
				pszSourceFilename, 
				(imageDataLen / getNumImageBytesRequired(quality)));

			rdr_close(hsec);
			pngrdr_close(himg);
			exit(-1);
		}

		imageData = (uint8_t *)malloc(imageDataLen);

		if (imageData == NULL) {
    		fprintf(stderr, "Could not allocate memory for image data\n");
			rdr_close(hsec);
			pngrdr_close(himg);
			exit(-1);
		}

		imageBytesRead = pngrdr_read(himg, imageData, imageDataLen);

		if (imageBytesRead < imageDataLen) {
			fprintf(stderr, "Expected %u bytes of image data, but got %u bytes\n", imageDataLen, imageBytesRead);
			rdr_close(hsec);
			pngrdr_close(himg);
			exit(-1);
		}

		pngrdr_close(himg);

		numImgBytesRequired = getNumImageBytesRequired(quality);

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

		pngwrtr_open(himg, pszOutputFilename);
		pngwrtr_write(himg, imageData, imageDataLen);
		pngwrtr_close(himg);

		dbg_free(imageData, __FILE__, __LINE__);
		dbg_free(himg, __FILE__, __LINE__);

    	rdr_close(hsec);

		dbg_free(secretDataBlock, __FILE__, __LINE__);
    }
    else {
		/*
		** Extract our secret file from the source image...
		*/
		himg = pngrdr_open(pszSourceFilename);

		if (himg == NULL) {
    		fprintf(stderr, "Could not open source image file %s: %s\n", pszSourceFilename, strerror(errno));
    		exit(-1);
		}

		imageDataLen = pngrdr_get_data_length(himg);

		imageData = (uint8_t *)malloc(imageDataLen);

		if (imageData == NULL) {
    		fprintf(stderr, "Could not allocate memory for image data\n");
			pngrdr_close(himg);
			exit(-1);
		}

		imageBytesRead = pngrdr_read(himg, imageData, imageDataLen);

		pngrdr_close(himg);

		dbg_free(himg, __FILE__, __LINE__);

		hsec = wrtr_open(pszOutputFilename, algo);

		if (hsec == NULL) {
			fprintf(stderr, "Failed to open output file %s\n", pszOutputFilename);
			dbg_free(imageData, __FILE__, __LINE__);
			exit(-1);
		}

		secretDataBlockLen = wrtr_get_block_size(hsec);
		secretDataBlock = (uint8_t *)malloc(secretDataBlockLen);

		if (secretDataBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for secret data block\n");
			dbg_free(imageData, __FILE__, __LINE__);
			exit(-1);
		}

		if (algo == aes256) {
			if (wrtr_set_key_aes(hsec, key, keyLength)) {
				fprintf(stderr, "Failed to set AES key\n");
				dbg_free(imageData, __FILE__, __LINE__);
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
					dbg_free(secretDataBlock, __FILE__, __LINE__);
					dbg_free(imageData, __FILE__, __LINE__);
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

		dbg_free(imageData, __FILE__, __LINE__);
		dbg_free(secretDataBlock, __FILE__, __LINE__);
    }

	if (algo == aes256) {
		secureFree(key, keyBufferLen);
	}

	return 0;
}
