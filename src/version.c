#include "version.h"

#define __BDATE__      "2022-07-05 21:17:36"
#define __BVERSION__   "1.2.001"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
