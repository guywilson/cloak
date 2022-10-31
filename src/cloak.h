#include <stdint.h>

#include "secretrw.h"

#ifndef __INCL_CLOAK
#define __INCL_CLOAK

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

uint32_t    getKey(uint8_t * keyBuffer, uint32_t keyBufferLength, const char * pwd);
uint8_t     getBitMask(merge_quality quality);
int         getNumImageBytesRequired(merge_quality quality);
void        mergeSecretByte(
                    uint8_t * imageBytes, 
                    int numImageBytes, 
                    uint8_t secretByte, 
                    merge_quality quality);
uint8_t     extractSecretByte(
                    uint8_t * imageBytes, 
                    uint32_t numImageBytes, 
                    merge_quality quality);
uint32_t    getImageCapacity(char * pszInputImageFile, merge_quality quality);
int         merge(
                const char * pszInputImageFile, 
                const char * pszSecretFile, 
                const char * pszKeystreamFile,
                const char * pszOutputImageFile,
                merge_quality quality, 
                encryption_algo algo, 
                uint8_t * key, 
                uint32_t keyLength);
int         extract(
                const char * pszInputImageFile, 
                const char * pszKeystreamFile,
                const char * pszSecretFile, 
                merge_quality quality, 
                encryption_algo algo, 
                uint8_t * key, 
                uint32_t keyLength);

#endif
