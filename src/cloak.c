#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cloak_types.h"
#include "secretrw.h"
#include "pngrw.h"
#include "random_block.h"
#include "utils.h"

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

uint32_t getKey(uint8_t * keyBuffer, uint32_t keyBufferLength)
{
	memcpy(keyBuffer, random_block, 32U);

	return 32U;
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
	uint8_t *		key;
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
	}
    
	key = (uint8_t *)malloc(keyBufferLen);

	if (key == NULL) {
		fprintf(stderr, "Failed to allocate memory for key\n");
		exit(-1);
	}

	keyLength = getKey(key, keyBufferLen);

	HSECRW			hsec;
	HPNG			hpng;
	uint8_t *		secretDataBlock;
	uint8_t *		rowBuffer;
	uint8_t *		imageData;
	uint8_t			imageBuffer[8];
	uint8_t			secretByte = 0x00;
	uint32_t		rowBufferLen;
	uint32_t		imageDataLen;
	uint32_t		secretBytesRemaining = 0;
	uint32_t		imageBytesRead;
	boolean			hasMoreDataToMerge = true;
	boolean			deferMerge = false;
	int				numImgBytesRequired = 0;
	int				numImgBytesUsed = 0;
	int				rowBufferIndex = 0;
	int				secretBufferIndex = 0;
	int				deferredMergeIndex = 0;
	int				deferredBytesLeft = 0;

    if (isMerge) {
		hsec = rdr_open(pszInputFilename, key, keyLength, BLOCK_SIZE, algo);

    	if (hsec == NULL) {
    		fprintf(stderr, "Could not open input file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
    	}
    	
		if (algo == xor) {
			rdr_set_keystream_file(hsec, pszKeystreamFilename);
		}

    	secretDataBlock = (uint8_t *)malloc(BLOCK_SIZE);
    	
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

		printf("Read %u bytes of image data, expected %u bytes\n", imageBytesRead, imageDataLen);

		rowBufferLen = pngrw_get_row_buffer_len(hpng);

		rowBuffer = (uint8_t *)malloc(rowBufferLen);

		if (rowBuffer == NULL) {
    		fprintf(stderr, "Could not allocate memory for image row buffer\n");
			rdr_close(hsec);
			pngrw_close(hpng);
			exit(-1);
		}

		numImgBytesRequired = getNumImageBytesRequired(quality);

		while (pngrw_has_more_rows(hpng)) {
			if (pngrw_read_row(hpng, rowBuffer, rowBufferLen)) {
				break;
			}

			// printf("\nRead PNG row %u bytes long\n", rowBufferLen);
			// hexDump(rowBuffer, rowBufferLen);

			for (rowBufferIndex = 0;
				rowBufferIndex < rowBufferLen;
				rowBufferIndex += numImgBytesUsed)
			{
				if (hasMoreDataToMerge) {
					if (secretBytesRemaining == 0) {
						if (rdr_has_more_blocks(hsec)) {
							secretBytesRemaining = rdr_read_block(hsec, secretDataBlock);
							secretBufferIndex = 0;

							printf("\nRead secret block %u bytes long\n", secretBytesRemaining);
							hexDump(secretDataBlock, secretBytesRemaining);
						}
						else {
							hasMoreDataToMerge = false;
						}
					}

					if (rowBufferIndex <= (rowBufferLen - numImgBytesRequired)) {
						/*
						** We have sufficient image bytes to do the merge...
						*/
						if (deferMerge) {
							printf("Copying deferred %d bytes at index %d\n", deferredBytesLeft, deferredMergeIndex);
							__getch();

							memcpy(
								&imageBuffer[deferredMergeIndex], 
								&rowBuffer[rowBufferIndex], 
								deferredBytesLeft);

							printf("Complete image buffer (post defer):\n");
							printf(
								"%02X%02X %02X%02X %02X%02X %02X%02X\n", 
								imageBuffer[0], 
								imageBuffer[1], 
								imageBuffer[2], 
								imageBuffer[3], 
								imageBuffer[4], 
								imageBuffer[5], 
								imageBuffer[6], 
								imageBuffer[7]);

							/*
							** Temporarily set this to how many bytes we've 
							** just copied, we will reset it the next time 
							** around...
							*/
							numImgBytesUsed = deferredBytesLeft;

							deferMerge = false;
						}
						else {
							memcpy(
								imageBuffer, 
								&rowBuffer[rowBufferIndex], 
								numImgBytesRequired);

							numImgBytesUsed = numImgBytesRequired;
						}

						secretByte = secretDataBlock[secretBufferIndex++];
						secretBytesRemaining--;

						mergeSecretByte(
								imageBuffer, 
								numImgBytesRequired, 
								secretByte, 
								quality);

						/*
						** Copy the merged bytes back to the row buffer ready for writing...
						*/
						memcpy(
							&rowBuffer[rowBufferIndex], 
							imageBuffer, 
							numImgBytesRequired);

						memset(imageBuffer, 0x00, numImgBytesRequired);
					}
					else {
						/*
						** We don't have enough image bytes, so defer the merge 
						** until we get some more... 
						*/
						deferMerge = true;
						deferredBytesLeft = numImgBytesRequired - (rowBufferLen - rowBufferIndex);

						printf("rowBufferLen: %d, rowBufferIndex: %d\n", rowBufferLen, rowBufferIndex);
						printf("Need to defer merge, copying %d bytes and deferring %d bytes\n", (numImgBytesRequired - deferredBytesLeft), deferredBytesLeft);
						__getch();

						/*
						** Copy what is left in the row buffer and save
						** the index so we know where to start when we
						** have more data...
						*/
						memcpy(
							imageBuffer, 
							&rowBuffer[rowBufferIndex], 
							(numImgBytesRequired - deferredBytesLeft));

						deferredMergeIndex = 
									numImgBytesRequired - 
									deferredBytesLeft;

						/*
						** Temporarily set this to how many bytes we've 
						** just copied, we will reset it the next time 
						** around...
						*/
						numImgBytesUsed = 
									numImgBytesRequired - 
									deferredBytesLeft;
					}
				}
			}

			if (!deferMerge) {
				pngrw_write_row(hpng, rowBuffer, rowBufferLen);
			}
		}

		free(secretDataBlock);
		free(rowBuffer);
    	
    	rdr_close(hsec);
		pngrw_close(hpng);
    }
    else {
    }

	secureFree(key, keyBufferLen);
}
