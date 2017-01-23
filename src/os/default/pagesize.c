#include <unistd.h>

long os_getpagesize()
{
    return sysconf(_SC_PAGESIZE);
}

