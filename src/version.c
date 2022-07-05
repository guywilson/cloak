#include "version.h"

#define __BDATE__      "2022-07-05 12:57:46"
#define __BVERSION__   "1.1.001"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
