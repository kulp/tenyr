#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *os_fopen(const char *path, const char *mode)
{
    FILE *f = NULL;

    if (path[0] == '/') {
        // absolute path, needs to be construed relative to mountpoint
        size_t need = 1 + snprintf(NULL, 0, MOUNT_POINT "%s", path);
        char *buf = malloc(need);
        snprintf(buf, need, MOUNT_POINT "%s", path);
        f = fopen(buf, mode);
        free(buf);
    } else {
        // default behaviour, os_preamble() has already chdir()ed correctly
        f = fopen(path, mode);
    }

    return f;
}

