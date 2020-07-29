#define _XOPEN_SOURCE (700) /* for strdup */

#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

#include "os_common.h"

// Return nonzero from path_walker to end walking
typedef int path_walker(size_t len, const char *name, void *ud);
// Returns walker's last result (stops walking when walker returns true), or
// returns 0 if PATH is empty or missing.
int os_walk_path_search_list(path_walker walker, void *ud)
{
    int found = 0;

    const char *pathstr = getenv("PATH");
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

static int walker(size_t len, const char *name, void *ud)
{
    const char **paths = ud;
    const char *argv0 = paths[0];
    size_t alen = strlen(argv0);
    char *buf = calloc(1,len + alen + 1 + 1);
    strncpy(buf, name, len);
    buf[len] = '/';
    strncpy(&buf[len + 1], argv0, alen);
    if (access(buf, F_OK) == 0) {
        paths[1] = buf;
        return 1;
    }

    free(buf);
    return 0;
}

char *os_find_self(const char *argv0)
{
    // Find the first file in PATH, or return argv[0]
    const char *paths[] = { argv0, NULL };
    if (os_walk_path_search_list(walker, paths) != 0)
        return strdup(paths[1]);

    return strdup(argv0);
}

