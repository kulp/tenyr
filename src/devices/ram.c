#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "ram.h"

struct ram_state {
    size_t memsize;
    int32_t *mem;
    int32_t base;
};

static int ram_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    struct ram_state *ram = *(void**)cookie;

    if (!ram) {
        int32_t base = device->bounds[0];
        int32_t end  = device->bounds[1];
        ram = *(void**)cookie = malloc(sizeof *ram);
        ram->memsize = end - base + 1;
        ram->mem = malloc(ram->memsize * sizeof *ram->mem);
        ram->base = base;
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

static int ram_op(void *cookie, int op, int32_t addr, int32_t *data)
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

int ram_add_device_sized(struct device *device, int32_t base, int32_t len)
{
    *device = (struct device){
        .bounds = { base, base + len - 1 },
        .ops = {
            .op = ram_op,
            .init = ram_init,
            .fini = ram_fini,
        },
    };

    return 0;
}

int ram_add_device(struct device *device)
{
    return ram_add_device_sized(device, RAM_BASE, RAM_END - RAM_BASE + 1);
}

/* vi: set ts=4 sw=4 et: */
