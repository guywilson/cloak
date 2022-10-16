#include "version.h"

#define __BDATE__      "2022-10-16 15:58:49"
#define __BVERSION__   "1.2.006"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
