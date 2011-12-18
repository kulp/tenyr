#include <assert.h>
#include <stdint.h>
#include <search.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"

struct ram_state {
    int32_t mem[1 << 24];
};

static int ram_init(void *cookie, ...)
{
    struct ram_state *ram = *(void**)cookie = malloc(sizeof *ram);
    memset(ram->mem, 0, sizeof ram->mem);

    return 0;
}

static int ram_fini(void *cookie, ...)
{
    struct ram_state *ram = cookie;
    free(ram);

    return 0;
}

static int ram_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct ram_state *ram = cookie;
    assert(("Address within address space", !(addr & ~PTR_MASK)));

    // TODO elucidate ops (right now 1=Write, 0=Read)
    if (op == 1)
        ram->mem[addr] = *data;
    else if (op == 0)
        *data = ram->mem[addr];
    else
        return 1;

    return 0;
}

int ram_add_device(struct device *device)
{
    *device = (struct device){
        .bounds[0] = 0,
        .bounds[1] = (1 << 24) - 1,
        .op = ram_op,
        .init = ram_init,
        .fini = ram_fini,
    };

    return 0;
}

