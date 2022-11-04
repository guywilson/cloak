#include "version.h"

#define __BDATE__      "2022-11-04 23:03:37"
#define __BVERSION__   "2.0.001"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
