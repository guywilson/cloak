#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_PNGRW
#define __INCL_PNGRW

struct _png_handle;
typedef struct _png_handle *    HPNG;

HPNG        pngrw_open(char * pszSourceImageName, char * pszTargetImageName);
void        pngrw_read_close(HPNG hpng);
void        pngrw_write_close(HPNG hpng);
uint32_t    pngrw_get_row_buffer_len(HPNG hpng);
uint32_t    pngrw_get_data_length(HPNG hpng);
boolean     pngrw_has_more_rows(HPNG hpng);
uint32_t    pngrw_read(HPNG hpng, uint8_t * data, uint32_t dataLength);
int         pngrw_read_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength);
int         pngrw_write_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength);
uint32_t    pngrw_write(HPNG hpng, uint8_t * data, uint32_t dataLength);

#endif
