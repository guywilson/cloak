#include "version.h"

#define __BDATE__      "2023-10-26 15:19:57"
#define __BVERSION__   "2.2.001"

const char * getVersion(void) {
    return __BVERSION__;
}

const char * getBuildDate(void) {
    return __BDATE__;
}
