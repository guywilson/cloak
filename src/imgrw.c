#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#include <png.h>

#include "imgrw.h"
#include "cloak_types.h"

#define __BMP_WIN32_HEADER_SIZE                     40
#define __BMP_OS21X_HEADER_SIZE                     12

typedef enum image_type {
    img_win32bitmap,
    img_png,
    img_unknown
}
img_type;

struct _img_handle {
    /*
    ** Common attributes...
    */
    FILE *      fptr_input;
    FILE *      fptr_output;

    img_type    type;
    uint32_t    rowCounter;

    uint32_t	width;
    uint32_t	height;
    uint8_t     channels;
    uint16_t    bitsPerPixel;

    /*
    ** PNG specific attributes...
    */
    png_structp png_ptr_read;
    png_structp png_ptr_write;
	png_infop	info_ptr_read;
	png_infop	info_ptr_write;

    int         colourType;
    int         bitDepth;

    /*
    ** BMP specific attributes...
    */
};

typedef struct __attribute__((__packed__))
{
    char            bm[2];
	uint32_t        fileSize;
	uint32_t        reserved;
	uint32_t        dataOffset;
	
	uint32_t        dibSize;
	int32_t         width;
	int32_t         height;
	uint16_t        colourPlanes;
	uint16_t        bitsPerPixel;
	uint32_t        compressionMethod;
	uint32_t        rawDataLength;
	int32_t         horizontalResolution;
	int32_t         verticalResolution;
	uint32_t        numPaletteColours;
	uint32_t        numImportantColours;
}
BMP_HEADER;

// Global
jmp_buf		    jmpbuf;


img_type _getImageType(FILE * fptr)
{
    size_t          currentPos;
    img_type        type;
    uint8_t         header[18];
    uint32_t        dibSize;

    currentPos = ftell(fptr);

    fseek(fptr, 0, SEEK_SET);
    fread(header, 1, 18, fptr);
    fseek(fptr, currentPos, SEEK_SET);

    /*
    ** Get the size of the header...
    */
    memcpy(&dibSize, &header[14], 4);

    if (header[0] == 'B' && header[1] == 'M') {
        if (dibSize == __BMP_WIN32_HEADER_SIZE) {
            type = img_win32bitmap;
        }
        else {
            type = img_unknown;
        }
    }
    else if (header[1] == 'P' && header[2] == 'N' && header[3] == 'P') {
        type = img_png;
    }
    else {
        type = img_unknown;
    }

    return type;
}

void _readwrite_error_handler(png_structp png_ptr, png_const_charp msg)
{
    fprintf(stderr, "writepng libpng error: %s\n", msg);
    fflush(stderr);

    longjmp(jmpbuf, 1);
}


HIMG pngrdr_open(char * pszImageName)
{
    HIMG            himg;

    himg = (HIMG)malloc(sizeof(struct _img_handle));

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        return NULL;
    }

    himg->fptr_input = fopen(pszImageName, "rb");
    
    if (himg->fptr_input == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

	himg->png_ptr_read = png_create_read_struct(
                                    PNG_LIBPNG_VER_STRING,
                                    himg, 
                                    _readwrite_error_handler, 
                                    NULL);

	if (himg->png_ptr_read == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return NULL;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	himg->info_ptr_read = png_create_info_struct(himg->png_ptr_read);
	
    if (himg->info_ptr_read == NULL) {
	  png_destroy_read_struct(&himg->png_ptr_read, NULL, NULL);
	  return NULL;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	* the normal method of doing things with libpng).  REQUIRED unless you
	* set up your own error handlers in the png_create_read_struct() earlier.
	*/

	if (setjmp(jmpbuf)) {
	  /* Free all of the memory associated with the png_ptr_read and info_ptr_read */
	  png_destroy_read_struct(&himg->png_ptr_read, &himg->info_ptr_read, NULL);

	  /* If we get here, we had a problem reading the file */
	  return NULL;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(himg->png_ptr_read, himg->fptr_input);
	
	png_read_info(himg->png_ptr_read, himg->info_ptr_read);

	himg->width =           png_get_image_width(himg->png_ptr_read, himg->info_ptr_read);
	himg->height =          png_get_image_height(himg->png_ptr_read, himg->info_ptr_read);
    himg->channels =        png_get_channels(himg->png_ptr_read, himg->info_ptr_read);
    himg->bitDepth =        png_get_bit_depth(himg->png_ptr_read, himg->info_ptr_read);
	himg->colourType =      png_get_color_type(himg->png_ptr_read, himg->info_ptr_read);

    himg->bitsPerPixel = himg->bitDepth * himg->channels;

    if(himg->bitDepth == 16) {
        png_set_strip_16(himg->png_ptr_read);
    }

    if(himg->colourType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(himg->png_ptr_read);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(himg->colourType == PNG_COLOR_TYPE_GRAY && himg->bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(himg->png_ptr_read);
    }

    if( himg->colourType == PNG_COLOR_TYPE_GRAY ||
        himg->colourType == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(himg->png_ptr_read);
    }

    png_read_update_info(himg->png_ptr_read, himg->info_ptr_read);

    if (himg->bitsPerPixel != 24) {
        fprintf(stderr, "PNG image must be 24-bit RGB\n");
        exit(-1);
    }
    
    himg->rowCounter = 0;

    return himg;
}

int pngwrtr_open(HIMG himg, char * pszImageName)
{
    himg->fptr_output = fopen(pszImageName, "wb");
    
    if (himg->fptr_output == NULL) {
        fprintf(stderr, "Could not open output image file %s: %s\n", pszImageName, strerror(errno));
        return -1;
    }

    himg->png_ptr_write = png_create_write_struct(
                                PNG_LIBPNG_VER_STRING, 
                                himg,
                                _readwrite_error_handler, 
                                NULL);
    
    if (himg->png_ptr_write == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return -1;
    }

    himg->info_ptr_write = png_create_info_struct(himg->png_ptr_write);
    
    if (himg->info_ptr_write == NULL) {
        png_destroy_write_struct(&himg->png_ptr_write, NULL);
        fprintf(stderr, "Failed to create PNG info struct\n");
        return -1;
    }

    png_init_io(himg->png_ptr_write, himg->fptr_output);

    png_set_compression_level(himg->png_ptr_write, 5);

    png_set_IHDR(
            himg->png_ptr_write, 
            himg->info_ptr_write, 
            himg->width, 
            himg->height,
            8, 
            PNG_COLOR_TYPE_RGB, 
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, 
            PNG_FILTER_TYPE_DEFAULT);

    png_write_info(himg->png_ptr_write, himg->info_ptr_write);

    himg->rowCounter = 0;

    return 0;
}

void pngrdr_close(HIMG himg)
{
	png_read_end(himg->png_ptr_read, NULL);
	png_destroy_read_struct(&himg->png_ptr_read, &himg->info_ptr_read, NULL);

    fclose(himg->fptr_input);
}

void pngwrtr_close(HIMG himg)
{
    png_write_end(himg->png_ptr_write, NULL);
    png_destroy_write_struct(&himg->png_ptr_write, &himg->info_ptr_write);

    fclose(himg->fptr_output);
}

uint32_t pngrdr_get_row_buffer_len(HIMG himg)
{
    return (uint32_t)png_get_rowbytes(himg->png_ptr_read, himg->info_ptr_read);
}

uint32_t pngwrtr_get_row_buffer_len(HIMG himg)
{
    return (uint32_t)png_get_rowbytes(himg->png_ptr_write, himg->info_ptr_write);
}

uint32_t pngrdr_get_data_length(HIMG himg)
{
    return (uint32_t)(pngrdr_get_row_buffer_len(himg) * himg->height);
}

boolean pngrw_has_more_rows(HIMG himg)
{
    return ((himg->rowCounter < himg->height) ? true : false);
}

uint32_t pngrdr_read(HIMG himg, uint8_t * data, uint32_t dataLength)
{
    uint32_t        index = 0;

    himg->rowCounter = 0;

    while (pngrw_has_more_rows(himg)) {
        if (pngrdr_read_row(himg, &data[index], (dataLength - index))) {
            fprintf(stderr, "Failed to read row...\n");
            exit(-1);
        }

        index += pngrdr_get_row_buffer_len(himg);
    }

    return index;
}

int pngrdr_read_row(HIMG himg, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngrdr_get_row_buffer_len(himg)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_read_row(himg->png_ptr_read, rowBuffer, NULL);

    himg->rowCounter++;

    return 0;
}

int pngwrtr_write_row(HIMG himg, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngwrtr_get_row_buffer_len(himg)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_write_row(himg->png_ptr_write, rowBuffer);

    himg->rowCounter++;

    return 0;
}

uint32_t pngwrtr_write(HIMG himg, uint8_t * data, uint32_t dataLength)
{
    uint32_t        index = 0;

    himg->rowCounter = 0;

    while (pngrw_has_more_rows(himg)) {
        if (pngwrtr_write_row(himg, &data[index], (dataLength - index))) {
            fprintf(stderr, "Failed to write row...\n");
            exit(-1);
        }

        index += pngwrtr_get_row_buffer_len(himg);
    }

    return index;
}

HIMG bmprdr_open(char * pszImageName)
{
    HIMG            himg;
    BMP_HEADER      header;

    himg = (HIMG)malloc(sizeof(struct _img_handle));

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        return NULL;
    }

    himg->fptr_input = fopen(pszImageName, "rb");
    
    if (himg->fptr_input == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

    /*
    ** Read the header...
    */
    fread(&header, 1, sizeof(BMP_HEADER), himg->fptr_input);

    /*
    ** Validate the bitmap...
    */
    if (header.bitsPerPixel != 24) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr_input);
        free(himg);
        return NULL;
    }
    if (header.compressionMethod != 0) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr_input);
        free(himg);
        return NULL;
    }
    if (header.numPaletteColours != 0) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr_input);
        free(himg);
        return NULL;
    }

    himg->bitsPerPixel = header.bitsPerPixel;
    himg->width = header.width;
    himg->height = header.height;
    himg->type = img_win32bitmap;

    /*
    ** Position the file pointer at the start of the image data...
    */
    fseek(himg->fptr_input, header.dataOffset, SEEK_SET);
   
    return himg;
}

int bmpwrtr_open(HIMG himg, char * pszImageName)
{
    return 0;
}

void bmprdr_close(HIMG himg)
{
    fclose(himg->fptr_input);

    free(himg);
}

uint32_t bmprdr_get_data_length(HIMG himg)
{
    uint32_t            dataLength;

    dataLength = (himg->width * 3);

    /*
    ** Add padding up to a 4 byte boundry...
    */
    dataLength += (dataLength % 4);
    dataLength *= himg->height;

    return dataLength;
}

uint32_t bmprdr_read(HIMG himg, uint8_t * data, uint32_t bufferLength)
{
    uint32_t            dataLength;
    uint32_t            bytesRead;

    dataLength = bmprdr_get_data_length(himg);

    if (bufferLength < dataLength) {
        fprintf(stderr, "Buffer must be at least %u bytes long", dataLength);
        return 0;
    }

    bytesRead = fread(data, 1, dataLength, himg->fptr_input);

    return bytesRead;
}
