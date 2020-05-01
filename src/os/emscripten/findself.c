#include "os_common.h"

#include <stdlib.h>

char *os_find_self(const char *argv0)
{
    (void)argv0;
    // This value is not used by its caller as of the time of writing -- this
    // implementation will likely be changed
    char *result = malloc(2);
    result[0] = '.';
    result[1] = '\0';
    return result;
}

