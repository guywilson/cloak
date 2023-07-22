#include "version.h"

#define __BDATE__      "2023-07-22 10:18:32"
#define __BVERSION__   "2.1.004"

const char * getVersion(void) {
    return __BVERSION__;
}

const char * getBuildDate(void) {
    return __BDATE__;
}
