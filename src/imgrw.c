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

#define HEADER_LOOKAHEAD_BUFFER_LEN                 18

#define HANDLE_POOL_SIZE                            8

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

typedef struct 
{
    int32_t         width;
    int32_t         height;

    uint8_t         channels;
    uint16_t        bitsPerPixel;
}
IMG_GEOMETRY;

struct _img_handle {
    /*
    ** Common attributes...
    */
    uint16_t        _id;
    img_type        type;

    FILE *          fptr;
    IMG_GEOMETRY    geometry;

    /*
    ** PNG specific attributes...
    */
    png_structp     png_ptr;
	png_infop	    info_ptr;

    int             colourType;
    int             bitDepth;
    uint32_t        rowCounter;

    /*
    ** BMP specific attributes...
    */
    BMP_HEADER *    pHeader;
};

struct _img_handle      _imageHandlePool[HANDLE_POOL_SIZE];
uint16_t                _nextId = 0x0000;

// Global
jmp_buf		    jmpbuf;

HIMG _allocateHandle()
{
    HIMG        himg = NULL;
    int         i;

    if (_nextId == 0x0000) {
        for (i = 0;i < HANDLE_POOL_SIZE;i++) {
            _imageHandlePool[i]._id = 0x0000;
        }

        _nextId++;
    }

    for (i = 0;i < HANDLE_POOL_SIZE;i++) {
        if (_imageHandlePool[i]._id == 0x0000) {
            himg = &_imageHandlePool[i];
            himg->_id = _nextId++;
            break;
        }
    }

    return himg;
}

void _freeHandle(HIMG himg)
{
    int         i;

    for (i = 0;i < HANDLE_POOL_SIZE;i++) {
        if (himg->_id == _imageHandlePool[i]._id) {
            himg->_id = 0x0000;
        }
    }
}

img_type _getImageType(char * pszImageName)
{
    FILE *          fptr_input;
    img_type        type;
    uint8_t         header[HEADER_LOOKAHEAD_BUFFER_LEN];
    uint32_t        dibSize;
    uint32_t        bytesRead;

    fptr_input = fopen(pszImageName, "rb");
    
    if (fptr_input == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

    bytesRead = fread(header, 1, HEADER_LOOKAHEAD_BUFFER_LEN, fptr_input);
    
    if (bytesRead < HEADER_LOOKAHEAD_BUFFER_LEN) {
        fprintf(stderr, "Failed to read image header from %s\n", pszImageName);
        exit(-1);
    }
    
    fclose(fptr_input);

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
    else if (header[1] == 'P' && header[2] == 'N' && header[3] == 'G') {
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

void imgrdr_copy_header(HIMG target, HIMG source)
{
    if (source->type == img_png) {
        memcpy(&target->geometry, &source->geometry, sizeof(IMG_GEOMETRY));
    }
    else if (source->type == img_win32bitmap) {
        memcpy(&target->geometry, &source->geometry, sizeof(IMG_GEOMETRY));
        memcpy(target->pHeader, source->pHeader, sizeof(BMP_HEADER));
    }
}

img_type imgrdr_get_type(HIMG himg)
{
    return himg->type;
}

HIMG imgrdr_open(char * pszImageName)
{
    img_type            type;

    type = _getImageType(pszImageName);

    if (type == img_png) {
        return pngrdr_open(pszImageName);
    }
    else if (type == img_win32bitmap) {
        return bmprdr_open(pszImageName);
    }
    else {
        fprintf(stderr, "Cannot open %s: Unsupported image type\n", pszImageName);
        return NULL;
    }
}

HIMG imgwrtr_open(char * pszImageName, img_type type)
{
    if (type == img_png) {
        return pngwrtr_open(pszImageName);
    }
    else if (type == img_win32bitmap) {
        return bmpwrtr_open(pszImageName);
    }

    return 0;
}

void imgrdr_close(HIMG himg)
{
    if (himg->type == img_png) {
        pngrdr_close(himg);
    }
    else if (himg->type == img_win32bitmap) {
        bmprdr_close(himg);
    }
}

void imgwrtr_close(HIMG himg)
{
    if (himg->type == img_png) {
        pngwrtr_close(himg);
    }
    else if (himg->type == img_win32bitmap) {
        bmpwrtr_close(himg);
    }
}

void imgrdr_destroy_handle(HIMG himg)
{
    _freeHandle(himg);
}

uint32_t imgrdr_get_data_length(HIMG himg)
{
    if (himg->type == img_png) {
        return pngrdr_get_data_length(himg);
    }
    else if (himg->type == img_win32bitmap) {
        return bmprdr_get_data_length(himg);
    }

    return 0;
}

uint32_t imgrdr_read(HIMG himg, uint8_t * data, uint32_t bufferLength)
{
    if (himg->type == img_png) {
        return pngrdr_read(himg, data, bufferLength);
    }
    else if (himg->type == img_win32bitmap) {
        return bmprdr_read(himg, data, bufferLength);
    }

    return 0;
}

int imgwrtr_write_header(HIMG himg)
{
    if (himg->type == img_png) {
        return pngwrtr_write_header(himg);
    }
    else if (himg->type == img_win32bitmap) {
        return bmpwrtr_write_header(himg);
    }

    return 0;
}

uint32_t imgwrtr_write(HIMG himg, uint8_t * data, uint32_t bufferLength)
{
    if (himg->type == img_png) {
        return pngwrtr_write(himg, data, bufferLength);
    }
    else if (himg->type == img_win32bitmap) {
        return bmpwrtr_write(himg, data, bufferLength);
    }

    return 0;
}

HIMG pngrdr_open(char * pszImageName)
{
    HIMG            himg;

    himg = _allocateHandle();

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        return NULL;
    }

    himg->fptr = fopen(pszImageName, "rb");
    
    if (himg->fptr == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        exit(-1);
    }

	himg->png_ptr = png_create_read_struct(
                                    PNG_LIBPNG_VER_STRING,
                                    himg, 
                                    _readwrite_error_handler, 
                                    NULL);

	if (himg->png_ptr == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return NULL;
	}

	/* Allocate/initialize the memory for image information.  REQUIRED. */
	himg->info_ptr = png_create_info_struct(himg->png_ptr);
	
    if (himg->info_ptr == NULL) {
	  png_destroy_read_struct(&himg->png_ptr, NULL, NULL);
	  return NULL;
	}

	/* Set error handling if you are using the setjmp/longjmp method (this is
	* the normal method of doing things with libpng).  REQUIRED unless you
	* set up your own error handlers in the png_create_read_struct() earlier.
	*/

	if (setjmp(jmpbuf)) {
	  /* Free all of the memory associated with the png_ptr_read and info_ptr_read */
	  png_destroy_read_struct(&himg->png_ptr, &himg->info_ptr, NULL);

	  /* If we get here, we had a problem reading the file */
	  return NULL;
	}

	/* Set up the input control if you are using standard C streams */
	png_init_io(himg->png_ptr, himg->fptr);
	
	png_read_info(himg->png_ptr, himg->info_ptr);

	himg->geometry.width =      png_get_image_width(himg->png_ptr, himg->info_ptr);
	himg->geometry.height =     png_get_image_height(himg->png_ptr, himg->info_ptr);
    himg->geometry.channels =   png_get_channels(himg->png_ptr, himg->info_ptr);
    himg->bitDepth =            png_get_bit_depth(himg->png_ptr, himg->info_ptr);
	himg->colourType =          png_get_color_type(himg->png_ptr, himg->info_ptr);

    himg->geometry.bitsPerPixel = himg->bitDepth * himg->geometry.channels;

    himg->type = img_png;

    if(himg->bitDepth == 16) {
        png_set_strip_16(himg->png_ptr);
    }

    if(himg->colourType == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(himg->png_ptr);
    }

    // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
    if(himg->colourType == PNG_COLOR_TYPE_GRAY && himg->bitDepth < 8) {
        png_set_expand_gray_1_2_4_to_8(himg->png_ptr);
    }

    if( himg->colourType == PNG_COLOR_TYPE_GRAY ||
        himg->colourType == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
        png_set_gray_to_rgb(himg->png_ptr);
    }

    png_read_update_info(himg->png_ptr, himg->info_ptr);

    if (himg->geometry.bitsPerPixel != 24) {
        fprintf(stderr, "PNG image must be 24-bit RGB\n");
        exit(-1);
    }
    
    himg->rowCounter = 0;

    return himg;
}

HIMG pngwrtr_open(char * pszImageName)
{
    HIMG            himg;

    himg = _allocateHandle();

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        return NULL;
    }

    himg->fptr = fopen(pszImageName, "wb");
    
    if (himg->fptr == NULL) {
        fprintf(stderr, "Could not open output image file %s: %s\n", pszImageName, strerror(errno));
        NULL;
    }

    himg->png_ptr = png_create_write_struct(
                                PNG_LIBPNG_VER_STRING, 
                                himg,
                                _readwrite_error_handler, 
                                NULL);
    
    if (himg->png_ptr == NULL) {
        fprintf(stderr, "Failed to create PNG write struct\n");
        return NULL;
    }

    himg->info_ptr = png_create_info_struct(himg->png_ptr);
    
    if (himg->info_ptr == NULL) {
        png_destroy_write_struct(&himg->png_ptr, NULL);
        fprintf(stderr, "Failed to create PNG info struct\n");
        return NULL;
    }

    png_init_io(himg->png_ptr, himg->fptr);

    png_set_compression_level(himg->png_ptr, 5);

    himg->rowCounter = 0;

    himg->type = img_png;

    return himg;
}

void pngrdr_close(HIMG himg)
{
	png_read_end(himg->png_ptr, NULL);
	png_destroy_read_struct(&himg->png_ptr, &himg->info_ptr, NULL);

    fclose(himg->fptr);
}

void pngwrtr_close(HIMG himg)
{
    png_write_end(himg->png_ptr, NULL);
    png_destroy_write_struct(&himg->png_ptr, &himg->info_ptr);

    fclose(himg->fptr);
}

uint32_t pngrdr_get_row_buffer_len(HIMG himg)
{
    return (uint32_t)png_get_rowbytes(himg->png_ptr, himg->info_ptr);
}

uint32_t pngwrtr_get_row_buffer_len(HIMG himg)
{
    return (uint32_t)png_get_rowbytes(himg->png_ptr, himg->info_ptr);
}

uint32_t pngrdr_get_data_length(HIMG himg)
{
    return (uint32_t)(pngrdr_get_row_buffer_len(himg) * himg->geometry.height);
}

boolean pngrw_has_more_rows(HIMG himg)
{
    return ((himg->rowCounter < himg->geometry.height) ? true : false);
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

    png_read_row(himg->png_ptr, rowBuffer, NULL);

    himg->rowCounter++;

    return 0;
}

int pngwrtr_write_row(HIMG himg, uint8_t * rowBuffer, uint32_t bufferLength)
{
    if (bufferLength < pngwrtr_get_row_buffer_len(himg)) {
        fprintf(stderr, "PNG row buffer is not long enough\n");
        return -1;
    }

    png_write_row(himg->png_ptr, rowBuffer);

    himg->rowCounter++;

    return 0;
}

int pngwrtr_write_header(HIMG himg)
{
    png_set_IHDR(
            himg->png_ptr, 
            himg->info_ptr, 
            himg->geometry.width, 
            himg->geometry.height,
            8, 
            PNG_COLOR_TYPE_RGB, 
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT, 
            PNG_FILTER_TYPE_DEFAULT);

    png_write_info(himg->png_ptr, himg->info_ptr);

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
    BMP_HEADER *    pHeader;
    uint32_t        bytesRead;

    pHeader = (BMP_HEADER *)malloc(sizeof(BMP_HEADER));

    if (pHeader == NULL) {
        fprintf(stderr, "Failed to allocate memory for bitmap header\n");
        return NULL;
    }

    himg = _allocateHandle();

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        free(pHeader);
        return NULL;
    }

    himg->fptr = fopen(pszImageName, "rb");
    
    if (himg->fptr == NULL) {
        fprintf(stderr, "Could not open input image file %s: %s\n", pszImageName, strerror(errno));
        free(pHeader);
        free(himg);
        exit(-1);
    }

    /*
    ** Read the header...
    */
    bytesRead = fread(pHeader, 1, sizeof(BMP_HEADER), himg->fptr);
    
    if (bytesRead < sizeof(BMP_HEADER)) {
        fprintf(stderr, "Could not read header from image file %s: %s\n", pszImageName, strerror(errno));
        free(pHeader);
        free(himg);
        exit(-1);
    }

    /*
    ** Validate the bitmap...
    */
    if (pHeader->bitsPerPixel != 24) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr);
        free(pHeader);
        free(himg);
        return NULL;
    }
    if (pHeader->compressionMethod != 0) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr);
        free(pHeader);
        free(himg);
        return NULL;
    }
    if (pHeader->numPaletteColours != 0) {
        fprintf(stderr, "Only 24-bit uncompressed RGB bitmaps are supported\n");
        fclose(himg->fptr);
        free(pHeader);
        free(himg);
        return NULL;
    }

    himg->geometry.bitsPerPixel = pHeader->bitsPerPixel;
    himg->geometry.width = pHeader->width;
    himg->geometry.height = pHeader->height;
    himg->type = img_win32bitmap;

    himg->pHeader = pHeader;

    /*
    ** Position the file pointer at the start of the image data...
    */
    fseek(himg->fptr, pHeader->dataOffset, SEEK_SET);
   
    return himg;
}

HIMG bmpwrtr_open(char * pszImageName)
{
    HIMG            himg;
    BMP_HEADER *    pHeader;

    pHeader = (BMP_HEADER *)malloc(sizeof(BMP_HEADER));

    if (pHeader == NULL) {
        fprintf(stderr, "Failed to allocate memory for bitmap header\n");
        return NULL;
    }

    himg = _allocateHandle();

    if (himg == NULL) {
        fprintf(stderr, "Failed to allocate memory for HIMG handle\n");
        free(pHeader);
        return NULL;
    }

    himg->pHeader = pHeader;

    himg->fptr = fopen(pszImageName, "wb");
    
    if (himg->fptr == NULL) {
        fprintf(stderr, "Could not open output image file %s: %s\n", pszImageName, strerror(errno));
        return NULL;
    }

    /*
    ** This should be the case anyhow, but start the image data
    ** immediately after the header...
    */
    himg->pHeader->dataOffset = sizeof(BMP_HEADER);

    himg->type = img_win32bitmap;

    return himg;
}

void bmprdr_close(HIMG himg)
{
    fclose(himg->fptr);
}

void bmpwrtr_close(HIMG himg)
{
    fclose(himg->fptr);
}

uint32_t bmprdr_get_data_length(HIMG himg)
{
    uint32_t            dataLength;

    dataLength = (himg->geometry.width * 3);

    /*
    ** Add padding up to a 4 byte boundry...
    */
    dataLength += (dataLength % 4);
    dataLength *= himg->geometry.height;

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

    bytesRead = fread(data, 1, dataLength, himg->fptr);

    return bytesRead;
}

int bmpwrtr_write_header(HIMG himg)
{
    uint32_t        bytesWritten;

    /*
    ** Write header...
    */
    bytesWritten = fwrite(himg->pHeader, 1, sizeof(BMP_HEADER), himg->fptr);

    if (bytesWritten < sizeof(BMP_HEADER)) {
        fprintf(stderr, "Failed to write bitmap header\n");
        free(himg->pHeader);
        fclose(himg->fptr);
        return -1;
    }

    free(himg->pHeader);

    return 0;
}

uint32_t bmpwrtr_write(HIMG himg, uint8_t * data, uint32_t bufferLength)
{
    uint32_t            dataLength;
    uint32_t            bytesWritten;

    dataLength = bmprdr_get_data_length(himg);

    if (bufferLength < dataLength) {
        fprintf(stderr, "Buffer must be at least %u bytes long", dataLength);
        return 0;
    }

    bytesWritten = fwrite(data, 1, dataLength, himg->fptr);

    return bytesWritten;
}
