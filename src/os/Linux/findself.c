#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdlib.h>

char *os_find_self(void)
{
    // PATH_MAX (used by readlink(2)) is not necessarily available
    size_t size = 4096;
    char *path = malloc(size);
    size_t used = readlink("/proc/self/exe", path, size);
    // need strictly less than size, or else we probably truncated
    if (used < size) {
        return path;
    } else {
        free(path);
        return NULL;
    }
}

