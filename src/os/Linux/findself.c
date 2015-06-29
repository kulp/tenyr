#include <unistd.h>
#include <limits.h>

char *os_find_self(void)
{
    // Using PATH_MAX is generally bad, but that's what readlink() says it uses.
    size_t size = PATH_MAX;
    char *path = malloc(size);
    if (readlink("/proc/self/exe", path, size) == 0) {
        return path;
    } else {
        free(path);
        return NULL;
    }
}

