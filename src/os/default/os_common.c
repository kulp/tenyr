#define _XOPEN_SOURCE 700
#include "os_common.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

int os_walk_path_search_list(path_walker walker, void *ud)
{
    int found = 0;

    char *pathstr = getenv("PATH");
    if (pathstr) {
        while (*pathstr != '\0') {
            static const char set[] = { PATH_SEPARATOR_CHAR, '\0' };
            size_t len = strcspn(pathstr, set);
            if (len == 0) {
                if (*pathstr != '\0')
                    pathstr++;
                continue;
            }
            if ((found = walker(len, pathstr, ud)))
                return found;
            pathstr += len;
        }
    }

    return found;
}

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

