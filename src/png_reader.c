#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <png.h>

#include "png_reader.h"

struct _png_handle {
    FILE *      fptrPNG;


};

HPNG png_open(char * pszPNGFilename)
{
    HPNG        hpng;

    hpng = (HPNG)malloc(sizeof(HPNG));

    if (hpng == NULL) {
        fprintf(stderr, "Failed to allocate memory for HPNG handle\n");
        return NULL;
    }

    hpng->fptrPNG = fopen(pszPNGFilename, "rb");

    return hpng;
}