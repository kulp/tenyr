#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdlib.h>

char *os_find_self(void)
{
    // PATH_MAX (used by readlink(2)) is not necessarily available
    size_t size = 2048, used = 0;
    char *path = NULL;
    do {
        size *= 2;
        path = realloc(path, size);
        used = readlink("/proc/self/exe", path, size);
        path[used - 1] = '\0';
    } while (used >= size && path != NULL);

    return path;
}

