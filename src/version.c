#include "version.h"

#define __BDATE__      "2024-05-03 08:54:18"
#define __BVERSION__   "2.2.002"

const char * getVersion(void) {
    return __BVERSION__;
}

const char * getBuildDate(void) {
    return __BDATE__;
}
