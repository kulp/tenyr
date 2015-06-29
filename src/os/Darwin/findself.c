#include <libproc.h>
#include <stdlib.h>
#include <unistd.h>

char *os_find_self(void)
{
    size_t size = PROC_PIDPATHINFO_MAXSIZE;
    char *path = malloc(size);
    if (proc_pidpath(getpid(), path, size) > 0) {
        return path;
    } else {
        free(path);
        return NULL;
    }
}

