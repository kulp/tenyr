#include "spi.h"

#include <stdio.h>

int spidummy_spi_init(void *pcookie)
{
    printf("%s(pcookie=%p)\n", __func__, pcookie);
    // point to self ; this will make tracing instances in debug output easy
    *(void**)pcookie = pcookie;
    return -1;
}

int spidummy_spi_select(void *cookie, int _ss)
{
    printf("%s(cookie=%p, _ss=%i)\n", __func__, cookie, _ss);
    return -1;
}

int spidummy_spi_clock(void *cookie, int _ss, int in, int *out)
{
    printf("%s(cookie=%p, _ss=%i, in=%i, out=%p)\n", __func__, cookie, _ss, in, (void*)out);
    *out = 0; // we are basically a set of disconnected wires
    return -1;
}

int spidummy_spi_fini(void *cookie)
{
    printf("%s(cookie=%p)\n", __func__, cookie);
    return -1;
}

