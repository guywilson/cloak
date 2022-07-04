#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <png.h>

#include "pngrw.h"
#include "cloak_types.h"

struct _png_handle {
    FILE *      fptr_input;
    FILE *      fptr_output;

    int			sample_depth;
	int			compression_level;
    uint32_t	width;
    uint32_t	height;
    uint8_t     channels;
    int         bitsPerPixel;
    int         colourType;

    uint32_t    rowCounter;

    png_structp png_ptr_read;
    png_structp png_ptr_write;
	png_infop	info_ptr_read;
	png_infop	info_ptr_write;
};

// Global
jmp_buf		    jmpbuf;

void _readwrite_error_handler(png_structp png_ptr, png_const_charp msg)
{
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

    longjmp(jmpbuf, 1);
}


HPNG pngrdr_open(char * pszImageName)
{
    HPNG            hpng;

    hpng = (HPNG)malloc(sizeof(HPNG));

    if (hpng == NULL) {
        fprintf(stderr, "Failed to allocate memory for HPNG handle\n");
        return NULL;
    }

    hpng->fptr_input = fopen(pszImageName, "rb");
    
    if (hpng->fptr_input == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

	hpng->png_ptr_read = png_create_read_struct(
					    PNG_LIBPNG_VER_STRING,
					    hpng, 
					    _readwrite_error_handler, 
					    NULL);

	if (hpng->png_ptr_read == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return NULL;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	hpng->info_ptr_read = png_create_info_struct(hpng->png_ptr_read);
	
    if (hpng->info_ptr_read == NULL) {
	  png_destroy_read_struct(&hpng->png_ptr_read, NULL, NULL);
	  return NULL;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	* the normal method of doing things with libpng).  REQUIRED unless you
	* set up your own error handlers in the png_create_read_struct() earlier.
	*/

	if (setjmp(jmpbuf)) {
	  /* Free all of the memory associated with the png_ptr_read and info_ptr_read */
	  png_destroy_read_struct(&hpng->png_ptr_read, &hpng->info_ptr_read, NULL);

	  /* If we get here, we had a problem reading the file */
	  return NULL;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(hpng->png_ptr_read, hpng->fptr_input);
	
	png_read_info(hpng->png_ptr_read, hpng->info_ptr_read);

	hpng->width =           png_get_image_width(hpng->png_ptr_read, hpng->info_ptr_read);
	hpng->height =          png_get_image_height(hpng->png_ptr_read, hpng->info_ptr_read);
    hpng->channels =        png_get_channels(hpng->png_ptr_read, hpng->info_ptr_read);
	hpng->bitsPerPixel =    png_get_bit_depth(hpng->png_ptr_read, hpng->info_ptr_read) * png_get_channels(hpng->png_ptr_read, hpng->info_ptr_read);
	hpng->colourType =      png_get_color_type(hpng->png_ptr_read, hpng->info_ptr_read);

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

HPNG pngwrtr_open(char * pszImageName)
{
    HPNG            hpng;

    hpng = (HPNG)malloc(sizeof(HPNG));

    if (hpng == NULL) {
        fprintf(stderr, "Failed to allocate memory for HPNG handle\n");
        return NULL;
    }
	
    hpng->fptr_output = fopen(pszImageName, "wb");
    
    if (hpng->fptr_output == NULL) {
        fprintf(stderr, "Could not open output image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

    hpng->png_ptr_write = png_create_write_struct(
                                PNG_LIBPNG_VER_STRING, 
                                hpng,
                                _readwrite_error_handler, 
                                NULL);
    
    if (hpng->png_ptr_write == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return NULL;
    }

    hpng->info_ptr_write = png_create_info_struct(hpng->png_ptr_write);
    
    if (hpng->info_ptr_write == NULL) {
        png_destroy_write_struct(&hpng->png_ptr_write, NULL);
        fprintf(stderr, "Failed to create PNG info struct\n");
        return NULL;
    }

    png_init_io(hpng->png_ptr_write, hpng->fptr_output);

    png_set_compression_level(hpng->png_ptr_write, 5);

    png_set_IHDR(
            hpng->png_ptr_write, 
            hpng->info_ptr_write, 
            hpng->width, 
            hpng->height,
            8, 
            PNG_COLOR_TYPE_RGB, 
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, 
            PNG_FILTER_TYPE_DEFAULT);

    png_write_info(hpng->png_ptr_write, hpng->info_ptr_write);

    hpng->rowCounter = 0;

    return hpng;
}

void pngrdr_close(HPNG hpng)
{
	png_read_end(hpng->png_ptr_read, NULL);
	png_destroy_read_struct(&hpng->png_ptr_read, &hpng->info_ptr_read, NULL);

    free(hpng);
}

void pngwrtr_close(HPNG hpng)
{
    png_write_end(hpng->png_ptr_write, NULL);
    png_destroy_write_struct(&hpng->png_ptr_write, &hpng->info_ptr_write);

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

uint32_t pngrw_get_data_length(HPNG hpng)
{
    return (uint32_t)(hpng->width * hpng->height * hpng->channels);
}

uint32_t pngrdr_read(HPNG hpng, uint8_t * data, uint32_t dataLength)
{
    uint32_t        index = 0;

    hpng->rowCounter = 0;

    while (pngrw_has_more_rows(hpng)) {
        if (pngrdr_read_row(hpng, &data[index], (dataLength - index))) {
            fprintf(stderr, "Failed to read row...\n");
            exit(-1);
        }

        index += pngrw_get_row_buffer_len(hpng);
    }

    return index;
}

int pngrdr_read_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngrw_get_row_buffer_len(hpng)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_read_row(hpng->png_ptr_read, rowBuffer, NULL);

    hpng->rowCounter++;

    return 0;
}

int pngwrtr_write_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngrw_get_row_buffer_len(hpng)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_write_row(hpng->png_ptr_write, rowBuffer);

    hpng->rowCounter++;

    return 0;
}

uint32_t pngwrtr_write(HPNG hpng, uint8_t * data, uint32_t dataLength)
{
    uint32_t        index = 0;

    hpng->rowCounter = 0;

    while (pngrw_has_more_rows(hpng)) {
        if (pngwrtr_write_row(hpng, &data[index], (dataLength - index))) {
            fprintf(stderr, "Failed to write row...\n");
            exit(-1);
        }

        index += pngrw_get_row_buffer_len(hpng);
    }

    return index;
}
