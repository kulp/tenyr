#include "os_common.h"

#include <windows.h>
#include <limits.h>
#include <stddef.h>

char *os_find_self(const char *argv0)
{
    size_t size = PATH_MAX;
    char *buf = malloc(size);
    if (GetModuleFileName(NULL, buf, size) > 0) {
        return buf;
    } else {
        free(buf);
        return NULL;
    }
}

