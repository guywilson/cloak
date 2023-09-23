#include "version.h"

#define __BDATE__      "2023-09-23 22:10:19"
#define __BVERSION__   "2.1.005"

const char * getVersion(void) {
    return __BVERSION__;
}

const char * getBuildDate(void) {
    return __BDATE__;
}
