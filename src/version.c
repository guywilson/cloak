#include "version.h"

#define __BDATE__      "2022-10-30 21:48:45"
#define __BVERSION__   "1.2.008"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
