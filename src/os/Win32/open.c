#include "os_common.h"

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

FILE *os_fopen(const char *path, const char *mode)
{
    return fopen(path, mode);
}

int os_set_non_blocking(FILE *stream)
{
    (void)stream;
    /* TODO support non-blocking (overlapped ?) file handles */
    return 1; // return failure for now
}

