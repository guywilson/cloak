#include "version.h"

#define __BDATE__      "2022-07-01 22:25:36"
#define __BVERSION__   "1.0.003"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
