#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "os_common.h"

static int walker(size_t len, const char *name, void *ud)
{
    char **paths = ud;
    char *argv0 = paths[0];
    size_t alen = strlen(argv0);
    char *buf = malloc(len + alen + 1 + 1);
    strncpy(buf, name, len);
    buf[len] = PATH_COMPONENT_SEPARATOR_CHAR;
    strncpy(&buf[len + 1], argv0, alen);
    if (access(buf, F_OK) == 0) {
        paths[1] = buf;
        return 1;
    }

    free(buf);
    return 0;
}

char *os_find_self(char *argv0)
{
    // If all else fails, find the first file in PATH
    char *paths[] = { argv0, NULL };
    if (os_walk_path_search_list(walker, paths) == 0)
        return NULL;

    return paths[1];
}

