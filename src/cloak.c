#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cloak_types.h"
#include "reader.h"
#include "pngrw.h"
#include "random_block.h"
#include "utils.h"

typedef enum {
	quality_high = 1,
	quality_medium = 2,
	quality_low = 4
}
merge_quality;

#define CLOAK_MERGE_QUALITY_HIGH				1
#define CLOAK_MERGE_QUALITY_MEDIUM				2
#define CLOAK_MERGE_QUALITY_LOW					4

#define BLOCK_SIZE								64


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

inline uint8_t getBitMask(merge_quality quality) {
	uint8_t mask = 0x00;

	for (int i = 0;i < quality;i++) {
		mask += (0x01 << i) & 0xFF;
	}

	return mask;
}

inline int getNumImageBytesRequired(merge_quality quality)
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

int main(int argc, char ** argv)
{
	int				i;
	int				quality = CLOAK_MERGE_QUALITY_HIGH;
	char *			arg;
	char *			pszExtension;
	char *			pszInputFilename = NULL;
	char *			pszKeystreamFilename = NULL;
	char *			pszOutputFilename = NULL;
	char *			pszSourceFilename = NULL;
	uint8_t *		key;
	uint32_t		keyLength;
	boolean			isMerge = false;
	FILE *			fOutput;
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
                    quality = atoi(argv[i + 1]);

                    switch (quality) {
                        case CLOAK_MERGE_QUALITY_HIGH:
                        case CLOAK_MERGE_QUALITY_MEDIUM:
                        case CLOAK_MERGE_QUALITY_LOW:
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
	}
    
    if (isMerge) {
		uint8_t *		inputBlock;
		uint8_t *		rowBuffer;
		uint32_t		rowBufferLen;
		uint32_t		bytesRead;
		HCLOAK			hc;
		HPNG			hpng;
		
    	fOutput = fopen(pszOutputFilename, "wb");
    	
    	if (fOutput == NULL) {
    		fprintf(stderr, "Could not open output file %s: %s\n", pszOutputFilename, strerror(errno));
    		exit(-1);
    	}

		key = (uint8_t *)random_block;
		keyLength = 32U;
    	
		hc = rdr_open(pszInputFilename, key, keyLength, BLOCK_SIZE, algo);

    	if (hc == NULL) {
    		fprintf(stderr, "Could not open input file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
    	}
    	
		if (algo == xor) {
			rdr_set_keystream_file(hc, pszKeystreamFilename);
		}

    	inputBlock = (uint8_t *)malloc(BLOCK_SIZE);
    	
    	if (inputBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for input block\n");
			rdr_close(hc);
			fclose(fOutput);
			exit(-1);
    	}
    	
		hpng = pngrw_open(pszSourceFilename, NULL);

		if (hpng == NULL) {
    		fprintf(stderr, "Could not open source image file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
		}

		rowBufferLen = pngrw_get_row_buffer_len(hpng);

		rowBuffer = (uint8_t *)malloc(rowBufferLen);

		if (rowBuffer == NULL) {
    		fprintf(stderr, "Could not allocate memory for image row buffer\n");
			rdr_close(hc);
			pngrw_close(hpng);
			fclose(fOutput);
			exit(-1);
		}

		uint32_t		secretBytesRemaining = 0;
		uint32_t		imgByteCounter;
		int				numImgBytesRequired = getNumImageBytesRequired(quality);
		uint8_t			imageBuffer[8];
		uint8_t			secretByte = 0x00;

		while (pngrw_has_more_rows(hpng)) {
			pngrw_read_row(hpng, rowBuffer, rowBufferLen);

			for (imgByteCounter = 0;imgByteCounter < rowBufferLen;imgByteCounter++) {
				if (secretBytesRemaining == 0) {
					if (rdr_has_more_blocks(hc)) {
						secretBytesRemaining = rdr_read_block(hc, inputBlock);
					}
				}

				mergeSecretByte(imageBuffer, numImgBytesRequired, secretByte, quality);
			}
		}

		while (rdr_has_more_blocks(hc)) {
			bytesRead = rdr_read_block(hc, inputBlock);

			printf("\nRead block %u bytes long\n", bytesRead);

			for (i = 0;i < bytesRead;i += 16) {
				printf(
					"%08X\t%02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X\n", 
					i,
					inputBlock[i], 
					inputBlock[i+1], 
					inputBlock[i+2], 
					inputBlock[i+3], 
					inputBlock[i+4], 
					inputBlock[i+5], 
					inputBlock[i+6], 
					inputBlock[i+7],
					inputBlock[i+8],
					inputBlock[i+9],
					inputBlock[i+10],
					inputBlock[i+11],
					inputBlock[i+12],
					inputBlock[i+13],
					inputBlock[i+14],
					inputBlock[i+15]
				);
			}
		}

		free(inputBlock);
    	
    	rdr_close(hc);
		pngrw_close(hpng);
    	fclose(fOutput);
    }
    else {
    }
}
