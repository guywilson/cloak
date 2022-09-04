#include "version.h"

#define __BDATE__      "2022-09-04 14:18:32"
#define __BVERSION__   "1.2.003"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
