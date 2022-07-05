#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_PNGRW
#define __INCL_PNGRW

struct _png_handle;
typedef struct _png_handle *    HPNG;

HPNG        pngrdr_open(char * pszImageName);
int         pngwrtr_open(HPNG hpng, char * pszImageName);
void        pngrdr_close(HPNG hpng);
void        pngwrtr_close(HPNG hpng);
uint32_t    pngrdr_get_row_buffer_len(HPNG hpng);
uint32_t    pngrdr_get_data_length(HPNG hpng);
boolean     pngrw_has_more_rows(HPNG hpng);
uint32_t    pngrdr_read(HPNG hpng, uint8_t * data, uint32_t dataLength);
int         pngrdr_read_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength);
int         pngwrtr_write_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength);
uint32_t    pngwrtr_write(HPNG hpng, uint8_t * data, uint32_t dataLength);

#endif
