#include "version.h"

#define __BDATE__      "2022-07-06 22:22:29"
#define __BVERSION__   "1.2.002"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
