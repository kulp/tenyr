#define _XOPEN_SOURCE 500
#include "os_common.h"

#include <unistd.h>
#include <stdlib.h>

char *os_find_self(const char *argv0)
{
    (void)argv0;
    // PATH_MAX (used by readlink(2)) is not necessarily available
    ssize_t size = 2048;
    ssize_t used = 0;
    char *path = NULL;
    do {
        size *= 2;
        path = realloc(path, (size_t)size);
        used = readlink("/proc/self/exe", path, (size_t)size);
        path[used] = '\0';
    } while (used >= size); // path has already been dereferencd - can't be NULL

    return path;
}

