#include "version.h"

#define __BDATE__      "2022-10-31 08:54:52"
#define __BVERSION__   "1.2.009"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
