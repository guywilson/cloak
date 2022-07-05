#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_PNGRW
#define __INCL_PNGRW

struct _img_handle;
typedef struct _img_handle *    HIMG;

HIMG        imgrdr_open(char * pszImageName);
int         imgwrtr_open(HIMG himg, char * pszImageName);
void        imgrdr_close(HIMG himg);
void        imgwrtr_close(HIMG himg);
uint32_t    imgrdr_get_data_length(HIMG himg);
uint32_t    imgrdr_read(HIMG himg, uint8_t * data, uint32_t bufferLength);
uint32_t    imgwrtr_write(HIMG himg, uint8_t * data, uint32_t bufferLength);

HIMG        pngrdr_open(char * pszImageName);
int         pngwrtr_open(HIMG himg, char * pszImageName);
void        pngrdr_close(HIMG himg);
void        pngwrtr_close(HIMG himg);
uint32_t    pngrdr_get_row_buffer_len(HIMG himg);
uint32_t    pngrdr_get_data_length(HIMG himg);
boolean     pngrw_has_more_rows(HIMG himg);
uint32_t    pngrdr_read(HIMG himg, uint8_t * data, uint32_t dataLength);
int         pngrdr_read_row(HIMG himg, uint8_t * rowBuffer, uint32_t bufferLength);
int         pngwrtr_write_row(HIMG himg, uint8_t * rowBuffer, uint32_t bufferLength);
uint32_t    pngwrtr_write(HIMG himg, uint8_t * data, uint32_t dataLength);

HIMG        bmprdr_open(char * pszImageName);
int         bmpwrtr_open(HIMG himg, char * pszImageName);
void        bmprdr_close(HIMG himg);
void        bmpwrtr_close(HIMG himg);
uint32_t    bmprdr_get_data_length(HIMG himg);
uint32_t    bmprdr_read(HIMG himg, uint8_t * data, uint32_t bufferLength);
uint32_t    bmpwrtr_write(HIMG himg, uint8_t * data, uint32_t bufferLength);

#endif
