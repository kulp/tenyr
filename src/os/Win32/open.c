#include "os_common.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

FILE *os_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

