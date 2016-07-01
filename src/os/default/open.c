#include <stdio.h>

FILE *os_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

