#include "os_common.h"

#include <stdlib.h>
#include <errno.h>

int os_get_tsimrc_path(char buf[], size_t sz)
{
    char *home = getenv("HOME");
    int need = snprintf(buf, sz, "%s/_tsimrc", home);
    if ((size_t)need >= sz) {
        errno = ENOSPC;
        return 1;
    }
    return need < 0; // true means failure
}

