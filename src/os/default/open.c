#define _XOPEN_SOURCE 700 /* for fileno */

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
    int fd = fileno(stream);
    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK))
        return 1;

    return 0;
}

