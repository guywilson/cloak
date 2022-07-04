#include "version.h"

#define __BDATE__      "2022-07-04 18:05:25"
#define __BVERSION__   "1.0.004"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
