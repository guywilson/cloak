#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <png.h>

#include "pngrw.h"
#include "cloak_types.h"

struct _png_handle {
    FILE *      fptrPNG;

    int			sample_depth;
	int			compression_level;
    uint32_t	width;
    uint32_t	height;
    uint8_t     channels;
    int         bitsPerPixel;
    int         colourType;

    uint32_t    rowCounter;

    png_structp png_ptr;
	png_infop	info_ptr;

    jmp_buf		jmpbuf;
};

void _readwrite_error_handler(png_structp png_ptr, png_const_charp msg)
{
	HPNG        hpng;

    /* This function, aside from the extra step of retrieving the "error
     * pointer" (below) and the fact that it exists within the application
     * rather than within libpng, is essentially identical to libpng's
     * default error handler.  The second point is critical:  since both
     * setjmp() and longjmp() are called from the same code, they are
     * guaranteed to have compatible notions of how big a jmp_buf is,
     * regardless of whether _BSD_SOURCE or anything else has (or has not)
     * been defined. */

    fprintf(stderr, "writepng libpng error: %s\n", msg);
    fflush(stderr);

    hpng = (HPNG)png_get_error_ptr(png_ptr);

    if (hpng == NULL) {         /* we are completely hosed now */
        fprintf(stderr,
          "writepng severe error:  jmpbuf not recoverable; terminating.\n");
        fflush(stderr);
        exit(99);
    }

    longjmp(hpng->jmpbuf, 1);
}


HPNG pngrw_open(char * pszSourceImageName, char * pszTargetImageName)
{
    HPNG            hpng;
    uint32_t        dataLength;

    hpng = (HPNG)malloc(sizeof(HPNG));

    if (hpng == NULL) {
        fprintf(stderr, "Failed to allocate memory for HPNG handle\n");
        return NULL;
    }

    hpng->fptrPNG = fopen(pszSourceImageName, "rb");

	hpng->png_ptr = png_create_read_struct(
					    PNG_LIBPNG_VER_STRING,
					    hpng, 
					    _readwrite_error_handler, 
					    NULL);

	if (hpng->png_ptr == NULL) {
	  return NULL;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	hpng->info_ptr = png_create_info_struct(hpng->png_ptr);
	
    if (hpng->info_ptr == NULL) {
	  png_destroy_read_struct(&hpng->png_ptr, NULL, NULL);
	  return NULL;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	* the normal method of doing things with libpng).  REQUIRED unless you
	* set up your own error handlers in the png_create_read_struct() earlier.
	*/

	if (setjmp(hpng->jmpbuf)) {
	  /* Free all of the memory associated with the png_ptr and info_ptr */
	  png_destroy_read_struct(&hpng->png_ptr, &hpng->info_ptr, NULL);

	  /* If we get here, we had a problem reading the file */
	  return NULL;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(hpng->png_ptr, hpng->fptrPNG);
	
	png_read_info(hpng->png_ptr, hpng->info_ptr);

	hpng->width =           png_get_image_width(hpng->png_ptr, hpng->info_ptr);
	hpng->height =          png_get_image_height(hpng->png_ptr, hpng->info_ptr);
    hpng->channels =        png_get_channels(hpng->png_ptr, hpng->info_ptr);
	hpng->bitsPerPixel =    png_get_bit_depth(hpng->png_ptr, hpng->info_ptr) * png_get_channels(hpng->png_ptr, hpng->info_ptr);
	hpng->colourType =      png_get_color_type(hpng->png_ptr, hpng->info_ptr);

    if (hpng->bitsPerPixel != 24) {
        fprintf(stderr, "PNG image must be 24-bit RGB\n");
        exit(-1);
    }
    if (hpng->colourType != PNG_COLOR_TYPE_RGB) {
        fprintf(stderr, "PNG image must be 24-bit RGB\n");
        exit(-1);
    }
    
    hpng->rowCounter = 0;

    return hpng;
}

void pngrw_close(HPNG hpng)
{
	png_read_end(hpng->png_ptr, NULL);
	
	  /* clean up after the read, and free any memory allocated - REQUIRED */
	png_destroy_read_struct(&hpng->png_ptr, &hpng->info_ptr, NULL);

    free(hpng);
}

uint32_t pngrw_get_row_buffer_len(HPNG hpng)
{
    return (uint32_t)(hpng->width * hpng->channels);
}

boolean pngrw_has_more_rows(HPNG hpng)
{
    return ((hpng->rowCounter < hpng->height) ? true : false);
}

int pngrw_read_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngrw_get_row_buffer_len(hpng)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_read_row(hpng->png_ptr, rowBuffer, NULL);

    hpng->rowCounter++;

    return 0;
}


