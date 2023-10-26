/******************************************************************************
Copyright (c) 2023 Guy Wilson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/
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

const char * pszWarranty =
    "\n\nCopyright (c) 2023 Guy Wilson\n\n" \

    "Permission is hereby granted, free of charge, to any person obtaining a copy\n" \
    "of this software and associated documentation files (the \"Software\"), to deal\n" \
    "in the Software without restriction, including without limitation the rights\n" \
    "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n" \
    "copies of the Software, and to permit persons to whom the Software is\n" \
    "furnished to do so, subject to the following conditions:\n\n" \

    "The above copyright notice and this permission notice shall be included in all\n" \
    "copies or substantial portions of the Software.\n\n" \

    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n" \
    "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n" \
    "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n" \
    "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n" \
    "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n" \
    "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n" \
    "SOFTWARE.\n\n";

static int _getProgNameStartPos(char * pszProgName) {
	int i = strlen(pszProgName);

	while (pszProgName[i] != '/' && i > 0) {
		i--;	
	}

	if (pszProgName[i] == '/') {
		i++;
	}

	return i;
}

static void printVersion(char * pszProgName) {
    printf(
		"%s version %s built: %s\n\n", 
		&pszProgName[_getProgNameStartPos(pszProgName)], 
		getVersion(), 
		getBuildDate());
}

static void printUsage(char * pszProgName) {
    printf("%s", pszWarranty);

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
	printf("             --generate-otp save OTP key to file specified with -k\n");
	printf("             --interactive interactive mode, all other arguments ignored\n");
#ifdef BUILD_GUI
	printf("             --gui launch app on startup, all other arguments ignored\n");
#endif
    printf("             --test=n where n is between 1 and 18 to run the numbered test case\n\n");
}

static char * promptStr(const char * pszPrompt, const size_t maxLength) {
    char        szLengthFormat[8];
    char        szFormat[8];
    char *      pszAnswer;

    pszAnswer = (char *)malloc(maxLength);

    if (pszAnswer == NULL) {
        return NULL;
    }

    sprintf(szLengthFormat, "%lu", maxLength - 1);
    sprintf(szFormat, "%%%ss", szLengthFormat);

    printf("%s", pszPrompt);
    fflush(stdout);

    scanf(szFormat, pszAnswer);
    fflush(stdin);

    return pszAnswer;
}

static char promptChar(const char * pszPrompt) {
    char        answer;

    printf("%s", pszPrompt);
    fflush(stdout);

    answer = getchar();
    fflush(stdin);

    return answer;
}

static boolean promptBool(const char * pszPrompt) {
    char answer = promptChar(pszPrompt);

    return (answer == 'y' || answer == 'Y');
}

int main(int argc, char ** argv) {
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
	uint32_t		otpLength = 0;
	boolean			isMerge = False;
	boolean			isReportSize = False;
	boolean			generateOTP = False;
    boolean         isInteractive = False;
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
                    break;
                }
#endif
                else if (strncmp(arg, "--interactive", 13) == 0) {
					isInteractive = True;
                    break;
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
                else if (strncmp(arg, "--generate-otp", 14) == 0) {
					generateOTP = True;
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

    if (isInteractive) {
        boolean         isValid = False;

        pszSourceFilename = 
            promptStr("Enter the image (24-bit bmp or png) you want to work with: ", 256);

        isValid = False;
        while (!isValid) {
            char mergeChar = 
                promptChar("Do you want to (h)ide or (e)xtract a secret file?: ");

            switch (mergeChar) {
                case 'h':
                    isMerge = True;
                    isValid = True;
                    break;

                case 'e':
                    isMerge = False;
                    isValid = True;
                    break;

                default:
                    isValid = False;
                    printf("Invalid action!\n\n");
                    break;
            }
        }

        char qualityChar;

        isValid = False;
        while (!isValid) {
            if (isMerge) {
                pszInputFilename = 
                    promptStr("Enter the secret file you want to hide in the image: ", 256);

                qualityChar = 
                    promptChar("Do you want hide your file with (h)igh, (m)edium, or (l)ow quality: ");

                pszOutputFilename = 
                    promptStr("Enter the output image file (PNG or BMP): ", 256);
            }
            else {
                pszOutputFilename = 
                    promptStr("Enter the secret file you want to extract from the image: ", 256);

                qualityChar = 
                    promptChar("Was the secret file hidden with (h)igh, (m)edium, or (l)ow quality: ");
            }

            switch (qualityChar) {
                case 'h':
                    quality = quality_high;
                    isValid = True;
                    break;

                case 'm':
                    quality = quality_medium;
                    isValid = True;
                    break;

                case 'l':
                    quality = quality_low;
                    isValid = True;
                    break;

                default:
                    isValid = False;
                    printf("Invald quality enetered!\n\n");
                    break;
            }
        }

        isValid = False;
        while (!isValid) {
            char algoChar = 
                promptChar("Which encryption scheme do you want to use - (a)es256, (x)or or (n)one?: ");

            switch (algoChar) {
                case 'a':
                    algo = aes256;
                    isValid = True;
                    break;

                case 'x':
                    algo = xor;

                    generateOTP = 
                        promptBool("Do you want me to generate a keystream file or use your own (y/n): ");

                    if (generateOTP) {
                        pszKeystreamFilename = 
                            promptStr("What keystream file name do you want me to write to: ", 256);
                    }
                    else {
                        pszKeystreamFilename = 
                            promptStr("What keystream file do you want to use (must be at least as big as the secret file): ", 256);
                    }

                    isValid = True;
                    break;

                case 'n':
                    algo = none;
                    isValid = True;
                    break;

                default:
                    isValid = False;
                    printf("Invalid encryption algorithm!\n\n");
                    break;
            }
        }
    }
    else {
        pszSourceFilename = strdup(argv[argc - 1]);
        
        if (pszInputFilename != NULL) {
            isMerge = True;
        }
    }

	if (algo == xor) {
		if (pszKeystreamFilename != NULL) {
			if (generateOTP) {
				otpLength = getFileSizeByName(pszInputFilename);
				generateKeystreamFile(pszKeystreamFilename, otpLength);
			}
		}
		else {
			printf("For encryption algorithm 'xor', you must specify a key stream file.\n");
			exit(-1);
		}
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

    free(pszSourceFilename);
    free(pszOutputFilename);

	return 0;
}
