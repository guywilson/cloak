#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "cloak.h"
#include "cloak_types.h"
#include "utils.h"
#include "test.h"


int fcompare(const char * pszFile1, const char * pszFile2)
{
    FILE *          fp1;
    FILE *          fp2;
    uint8_t         buf1[64];
    uint8_t         buf2[64];
    uint32_t        bytesRead1;
    uint32_t        bytesRead2;
    int             i;

    fp1 = fopen(pszFile1, "rb");
    fp2 = fopen(pszFile2, "rb");

    while (!feof(fp1) && !feof(fp2)) {
        bytesRead1 = fread(buf1, 1, 64, fp1);
        bytesRead2 = fread(buf2, 1, 64, fp2);

        if (bytesRead1 != bytesRead2) {
            fclose(fp1);
            fclose(fp2);

            return -1;
        }

        for (i = 0;i < bytesRead1;i++) {
            if (buf1[i] != buf2[i]) {
                fclose(fp1);
                fclose(fp2);

                return 1;
            }
        }
    }

    fclose(fp1);
    fclose(fp2);

    return 0;
}

int test(int testCase)
{
    const char *        pszPNGInputFile = "./test/flowers.png";
    const char *        pszPNGOutputFile = "./test/flowers_out.png";
    const char *        pszBMPInputFile = "./test/album.bmp";
    const char *        pszBMPOutputFile = "./test/album_out.bmp";
    const char *        pszSecretInputFile = "./test/README.md";
    const char *        pszSecretOutputFile = "./test/README.out";
    const char *        pszKeystream = "./test/rand.bin";
    encryption_algo     algo;
    merge_quality       quality;
    uint32_t            keyLength = 64U;
    uint8_t             key[keyLength];
    int                 failureCode = 0;

    switch (testCase) {
        case TEST_PNG_AES_HIGH:
            printf("Running test - File type: PNG; Encryption: AES; Quality: High\n");

            keyLength = getKey(key, 64U, "password");

            quality = quality_high;
            algo = aes256;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_AES_MED:
            printf("Running test - File type: PNG; Encryption: AES; Quality: Medium\n");
            
            keyLength = getKey(key, 64U, "password");

            quality = quality_medium;
            algo = aes256;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_AES_LOW:
            printf("Running test - File type: PNG; Encryption: AES; Quality: Low\n");
            
            keyLength = getKey(key, 64U, "password");

            quality = quality_low;
            algo = aes256;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_XOR_HIGH:
            printf("Running test - File type: PNG; Encryption: XOR; Quality: High\n");
            
            quality = quality_high;
            algo = xor;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_XOR_MED:
            printf("Running test - File type: PNG; Encryption: XOR; Quality: Medium\n");
            
            quality = quality_medium;
            algo = xor;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_XOR_LOW:
            printf("Running test - File type: PNG; Encryption: XOR; Quality: Low\n");
            
            quality = quality_low;
            algo = xor;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_NONE_HIGH:
            printf("Running test - File type: PNG; Encryption: None; Quality: High\n");
            
            quality = quality_high;
            algo = none;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_NONE_MED:
            printf("Running test - File type: PNG; Encryption: None; Quality: Medium\n");
            
            quality = quality_medium;
            algo = none;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_PNG_NONE_LOW:
            printf("Running test - File type: PNG; Encryption: None; Quality: Low\n");
            
            quality = quality_low;
            algo = none;

            merge(
                pszPNGInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszPNGOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszPNGOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_AES_HIGH:
            printf("Running test - File type: BMP; Encryption: AES; Quality: High\n");

            keyLength = getKey(key, 64U, "password");

            quality = quality_high;
            algo = aes256;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_AES_MED:
            printf("Running test - File type: BMP; Encryption: AES; Quality: Medium\n");

            keyLength = getKey(key, 64U, "password");

            quality = quality_medium;
            algo = aes256;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_AES_LOW:
            printf("Running test - File type: BMP; Encryption: AES; Quality: Low\n");

            keyLength = getKey(key, 64U, "password");

            quality = quality_low;
            algo = aes256;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                key, 
                keyLength);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                key,
                keyLength);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_XOR_HIGH:
            printf("Running test - File type: BMP; Encryption: XOR; Quality: High\n");
            
            quality = quality_high;
            algo = xor;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_XOR_MED:
            printf("Running test - File type: BMP; Encryption: XOR; Quality: Medium\n");
            
            quality = quality_medium;
            algo = xor;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_XOR_LOW:
            printf("Running test - File type: BMP; Encryption: XOR; Quality: Low\n");
            
            quality = quality_low;
            algo = xor;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                pszKeystream, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                pszKeystream,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_NONE_HIGH:
            printf("Running test - File type: BMP; Encryption: None; Quality: High\n");
            
            quality = quality_high;
            algo = none;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_NONE_MED:
            printf("Running test - File type: BMP; Encryption: None; Quality: Medium\n");
            
            quality = quality_medium;
            algo = none;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;

        case TEST_BMP_NONE_LOW:
            printf("Running test - File type: BMP; Encryption: None; Quality: Low\n");
            
            quality = quality_low;
            algo = none;

            merge(
                pszBMPInputFile, 
                pszSecretInputFile, 
                NULL, 
                pszBMPOutputFile, 
                quality, 
                algo, 
                NULL, 
                0U);

            extract(
                pszBMPOutputFile,
                NULL,
                pszSecretOutputFile,
                quality,
                algo,
                NULL,
                0U);

            failureCode = fcompare(pszSecretInputFile, pszSecretOutputFile);

            if (failureCode > 0) {
                printf("Test failed! Files are different\n");
            }
            else if (failureCode < 0) {
                printf("Test failed! Files are different sizes\n");
            }
            else {
                printf("Test passed!\n");
            }
            break;
    }

    return failureCode;
}
