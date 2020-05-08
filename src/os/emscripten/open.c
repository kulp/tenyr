#include "os_common.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

FILE *os_fopen(const char *path, const char *mode)
{
    FILE *f = NULL;

    if (path[0] == '/' && strncmp(path, "/dev", 4)) {
        // absolute path, needs to be construed relative to mountpoint
        int need = 1 + snprintf(NULL, 0, MOUNT_POINT "%s", path);
        char *buf = malloc((size_t)need);
        snprintf(buf, (size_t)need, MOUNT_POINT "%s", path);
        f = fopen(buf, mode);
        free(buf);
    } else {
        // either a relative path or a device path -- don't mangle path
        // default behaviour, os_preamble() has already chdir()ed correctly
        f = fopen(path, mode);
    }

    return f;
}

