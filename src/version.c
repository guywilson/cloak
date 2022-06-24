#include "version.h"

#define __BDATE__      "2022-06-24 11:27:26"
#define __BVERSION__   "1.0.001"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
