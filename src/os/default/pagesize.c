#include "os_common.h"

#include <unistd.h>

long os_getpagesize(void)
{
    return sysconf(_SC_PAGESIZE);
}

