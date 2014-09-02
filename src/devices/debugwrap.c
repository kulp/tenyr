#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "common.h"
#include "device.h"

struct debugwrap_state {
    struct device *wrapped;
};

static int debugwrap_init(struct plugin_cookie *pcookie, void *cookie, int nargs, ...)
{
    // our own init done in debugwrap_add_device ()
    struct debugwrap_state *debugwrap = *(void**)cookie;
    // TODO varargs ?
    debugwrap->wrapped->ops.init(pcookie, &debugwrap->wrapped->cookie, 0);
    return 0;
}

static int debugwrap_fini(void *cookie)
{
    // TODO destroy wrapped device ? or unwrap somehow ?
    struct debugwrap_state *debugwrap = cookie;
    debugwrap->wrapped->ops.fini(debugwrap->wrapped->cookie);
    free(debugwrap);

    return 0;
}

static int debugwrap_op(void *cookie, int op, uint32_t addr,
        uint32_t *data)
{
    struct debugwrap_state *debugwrap = cookie;

    debugwrap->wrapped->ops.op(debugwrap->wrapped->cookie, op, addr, data);
    printf("%-5s @ 0x%06x = %#x\n", op ? "write" : "read", addr, *data);

    return 0;
}

int debugwrap_add_device(struct device **device, struct device *wrap)
{
    struct debugwrap_state *debugwrap = malloc(sizeof *debugwrap);
    *debugwrap = (struct debugwrap_state){ .wrapped = wrap };

    **device = (struct device){
        .bounds = { wrap->bounds[0], wrap->bounds[1] },
        .ops = {
            .op = debugwrap_op,
            .init = debugwrap_init,
            .fini = debugwrap_fini,
        },
        .cookie = debugwrap,
    };

    return 0;
}

int debugwrap_wrap_device(struct device **device)
{
    struct device *copy = malloc(sizeof *copy);
    *copy = **device;

    return debugwrap_add_device(device, copy);
}

int debugwrap_unwrap_device(struct device *device)
{
    struct debugwrap_state *debugwrap = device->cookie;
    struct device *wrapped = debugwrap->wrapped;
    free(debugwrap);
    *device = *wrapped;

    return 0;
}

/* vi: set ts=4 sw=4 et: */
