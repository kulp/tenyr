#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "ram.h"

struct ram_state {
    uint32_t base;
    size_t memsize;
    uint32_t *mem;
};

static int ram_init(struct plugin_cookie *pcookie, void *cookie, int nargs, ...)
{
    int reset_memsize = 0;
    size_t memsize = RAM_END + 1;
    struct ram_state *ram = *(void**)cookie;

    if (!ram) {
        ram = *(void**)cookie = malloc(sizeof *ram);
        ram->mem = NULL;
        // start out by recording that we just used the default size, which can
        // be overridden explicitly (but an explicitly overridden size will not
        // be overridden by the default, if the default init() is called after
        // an explicit init())
        ram->memsize = 0;
        ram->base = RAM_BASE;
    }

    if (nargs > 0) {
        va_list vl;
        va_start(vl, nargs);

        ram->memsize = va_arg(vl, int);
        reset_memsize = 1;
        if (nargs > 1)
            ram->base = va_arg(vl, int);

        va_end(vl);
    }

    memsize = ram->memsize;

    // only reallocate if we were given an explicit size, or if we have no
    // memory yet
    if (!ram->mem || reset_memsize)
        ram->mem = realloc(ram->mem, ram->memsize * sizeof *ram->mem);

    for (unsigned long i = 0; i < memsize; i++)
        ram->mem[i] = 0xffffffff; // "illlegal" ; will trap

    return 0;
}

static int ram_fini(void *cookie)
{
    struct ram_state *ram = cookie;
    free(ram->mem);
    free(ram);

    return 0;
}

static int ram_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct ram_state *ram = cookie;

    if (op == OP_WRITE)
        ram->mem[addr - ram->base] = *data;
    else if (op == OP_READ)
        *data = ram->mem[addr - ram->base];
    else
        return 1;

    return 0;
}

int ram_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { RAM_BASE, RAM_END },
        .ops = {
            .op = ram_op,
            .init = ram_init,
            .fini = ram_fini,
        },
    };

    return 0;
}

