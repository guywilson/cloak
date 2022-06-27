#include <stdint.h>
#include "cloak_types.h"

#ifndef __INCL_PNGRW
#define __INCL_PNGRW

struct _png_handle;
typedef struct _png_handle *    HPNG;

HPNG        pngrw_open(char * pszSourceImageName, char * pszTargetImageName);
void        pngrw_close(HPNG hpng);
uint32_t    pngrw_get_row_buffer_len(HPNG hpng);
boolean     pngrw_has_more_rows(HPNG hpng);
int         pngrw_read_row(HPNG hpng, uint8_t * rowBuffer, uint32_t bufferLength);

#endif
