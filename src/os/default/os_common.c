#define _XOPEN_SOURCE 700
#include "os_common.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

char *build_path(const char *base, const char *fmt, ...)
{
    char *dir = strdup(base);
    char *solidus = strrchr(dir, PATH_COMPONENT_SEPARATOR_CHAR);
    if (solidus)
        solidus[1] = '\0';
    if (!fmt)
        return dir;

    va_list vl;
    va_start(vl, fmt);

    size_t flen = strlen(fmt) + strlen(dir) + 1;
    char *ff = malloc(flen);
    snprintf(ff, flen, "%s%s", solidus ? dir : "", fmt);
    size_t len = vsnprintf(NULL, 0, ff, vl);
    va_start(vl, fmt); // restart
    char *buf = malloc(len + 1);
    vsnprintf(buf, len + 1, ff, vl);

    free(ff);
    free(dir);
    va_end(vl);

    return buf;
}

