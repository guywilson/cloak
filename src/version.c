#include "version.h"

#define __BDATE__      "2022-10-29 22:46:42"
#define __BVERSION__   "1.2.007"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
