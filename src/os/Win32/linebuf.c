#include <stdio.h>

int os_set_buffering(FILE *stream, int mode)
{
    // Although setvbuf() is specified by C90, it appears to be useless on
    // Windows, or at least problematic under Wine.
    (void)stream;
    (void)mode;
    return 0;
}

