#include "os_common.h"

#include <stdio.h>

int os_set_buffering(FILE *stream, int mode)
{
    return setvbuf(stream, NULL, mode, 0);
}

