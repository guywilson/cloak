#include "version.h"

#define __BDATE__      "2022-11-18 10:06:37"
#define __BVERSION__   "2.1.002"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
