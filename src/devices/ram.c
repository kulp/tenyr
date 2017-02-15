#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "ram.h"

struct ram_state {
    uint32_t base;
    size_t memsize;
    uint32_t *mem;
};

static int ram_init(struct plugin_cookie *pcookie, void *cookie)
{
    struct ram_state *ram = *(void**)cookie;

    if (!ram) {
        ram = *(void**)cookie = malloc(sizeof *ram);
        ram->memsize = RAM_END + 1;
        ram->mem = malloc(ram->memsize * sizeof *ram->mem);
        ram->base = RAM_BASE;
    }

    // set all 32-bit words to 0xffffffff, the trap instruction
    memset(ram->mem, 0xff, ram->memsize * sizeof *ram->mem);

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
    else if (op == OP_INSN_READ || op == OP_DATA_READ)
        *data = ram->mem[addr - ram->base];
    else
        return 1;

    return 0;
}

int ram_add_device(struct device *device)
{
    *device = (struct device){
        .bounds = { RAM_BASE, RAM_END },
        .ops = {
            .op = ram_op,
            .init = ram_init,
            .fini = ram_fini,
        },
    };

    return 0;
}

/* vi: set ts=4 sw=4 et: */
