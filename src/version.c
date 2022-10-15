#include "version.h"

#define __BDATE__      "2022-10-15 21:40:43"
#define __BVERSION__   "1.2.005"

const char * getVersion()
{
    return __BVERSION__;
}

const char * getBuildDate()
{
    return __BDATE__;
}
