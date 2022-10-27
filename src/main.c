#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include <gcrypt.h>

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"
#include "version.h"

#define MAX_PASSWORD_LENGTH						255

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
	printf("             -s report image capacity then exit\n");
    printf("             --merge-quality=value where value is:\n");
	printf("                       'high', 'medium', or 'low'\n");
    printf("             --algo=value where value is:\n");
	printf("                    'aes' for AES-256 encryption (prompt for password),\n");
	printf("                    'xor' for one-time pad encryption (-k is mandatory),\n");
	printf("                    'none' for no encryption (hide only)\n\n");
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
	boolean			isReportSize = false;
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
                else if (strncmp(arg, "-s", 2) == 0) {
                    isReportSize = true;
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
    	else if (strcmp(pszExtension, "bmp") == 0) {
    		printf("Processing BMP image file %s\n", pszSourceFilename);
    	}
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

    if (isReportSize) {
        printf(
            "Image %s has a merge capacity of %u bytes at the specified quality\n", 
            pszSourceFilename, 
            getImageCapacity(pszSourceFilename, quality));
	}
	else if (isMerge) {
		merge(
			pszSourceFilename, 
			pszInputFilename, 
			pszKeystreamFilename, 
			pszOutputFilename, 
			quality, 
			algo, 
			key, 
			keyLength);
    }
    else {
		extract(
			pszSourceFilename, 
			pszKeystreamFilename, 
			pszOutputFilename, 
			quality, 
			algo, 
			key, 
			keyLength);
    }

	if (algo == aes256) {
		secureFree(key, keyBufferLen);
	}

	return 0;
}
