#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "cloak_types.h"
#include "reader.h"


#define CLOAK_MERGE_QUALITY_HIGH				1
#define CLOAK_MERGE_QUALITY_MEDIUM				2
#define CLOAK_MERGE_QUALITY_LOW					4

char * getFileExtension(char * pszFilename)
{
	char *			pszExt = NULL;
	int				i;
	
	i = strlen(pszFilename);

	while (i > 0) {
		if (pszFilename[i] == '.') {
			pszExt = &pszFilename[i + 1];
			break;
		}
		
		i--;
	}

	return pszExt;		
}

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
	boolean			isMerge = false;
	FILE *			fSource;
	FILE *			fOutput;
	FILE *			fKey;
	
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
    
    if (isMerge) {
		uint8_t *		inputBlock;
		uint32_t		bytesRead;
		uint32_t		blockSize;
		HCLOAK			hc;
		
    	fSource = fopen(pszSourceFilename, "rb");
    	
    	if (fSource == NULL) {
    		fprintf(stderr, "Could not open source file %s: %s\n", pszSourceFilename, strerror(errno));
    		exit(-1);
    	}
    	
    	fOutput = fopen(pszOutputFilename, "wb");
    	
    	if (fOutput == NULL) {
    		fprintf(stderr, "Could not open output file %s: %s\n", pszOutputFilename, strerror(errno));
    		exit(-1);
    	}
    	
		hc = rdr_open(pszInputFilename, 0, aes256);

    	if (hc == NULL) {
    		fprintf(stderr, "Could not open input file %s: %s\n", pszInputFilename, strerror(errno));
    		exit(-1);
    	}
    	
		blockSize = rdr_get_block_size(hc);

    	inputBlock = (uint8_t *)malloc(blockSize);
    	
    	if (inputBlock == NULL) {
    		fprintf(stderr, "Could not allocate memory for input block\n");
			rdr_close(hc);
			fclose(fOutput);
			fclose(fSource);
			exit(-1);
    	}
    	
		bytesRead = rdr_read_block(hc, inputBlock);
    	
    	
    	rdr_close(hc);
    	fclose(fOutput);
    	fclose(fSource);
    }
    else {
    }
}
