#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"
#include "test.h"
#include "version.h"

#ifdef BUILD_GUI
#include "gui.h"
#endif

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
	printf("                    'none' for no encryption (hide only)\n");
#ifdef BUILD_GUI
	printf("             --gui launch the GUI application on startup, all other arguments ignored\n");
#endif
    printf("             --test=n where n is between 1 and 18 to run the numbered test case\n\n");
}

int main(int argc, char ** argv)
{
	int				i;
    int             testNum = 0;
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
	uint32_t		keyLength = 0;
	boolean			isMerge = False;
	boolean			isReportSize = False;
	merge_quality	quality = quality_high;
	encryption_algo	algo = none;
#ifdef BUILD_GUI
	boolean			isGUI = False;
#endif

    if (argc > 0) {
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
                else if (strncmp(arg, "--test=", 7) == 0) {
                    testNum = atoi(&arg[7]);

                    return test(testNum);
                }
#ifdef BUILD_GUI
                else if (strncmp(arg, "--gui", 5) == 0) {
					isGUI = True;
                }
#endif
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
						free(pszAlgorithm);
						return -1;
					}

					free(pszAlgorithm);
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
						free(pszQuality);
						return -1;
					}

					free(pszQuality);
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
                    isReportSize = True;
                }
                else {
                    printf("Invalid option %s - %s --help for help", arg, argv[0]);
                    return -1;
                }
            }
        }
    }

#ifdef BUILD_GUI
	if (isGUI) {
		printf("Starting GUI...\n");
		return initiateGUI(1, argv);
	}
#endif

	if (algo == xor && pszKeystreamFilename == NULL) {
		printf("For encryption algorithm 'xor', you must specify a key stream file.\n");
		exit(-1);
	}

	pszSourceFilename = strdup(argv[argc - 1]);
	
	if (pszInputFilename != NULL) {
		isMerge = True;
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

		keyLength = getKey(key, keyBufferLen, NULL);
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
