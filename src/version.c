#include "version.h"

#define __BDATE__      "2024-05-03 10:00:09"
#define __BVERSION__   "2.2.003"

const char * getVersion(void) {
    return __BVERSION__;
}

const char * getBuildDate(void) {
    return __BDATE__;
}
