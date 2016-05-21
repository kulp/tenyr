#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "device.h"
#include "sim.h"

#define SERIAL_BASE (1ULL << 5)

static int serial_init(struct plugin_cookie *pcookie, void *cookie)
{
    return 0;
}

static int serial_fini(void *cookie)
{
    return 0;
}

static int serial_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    int rc = -1;

    if (op == OP_WRITE) {
        putchar(*data);
        fflush(stdout);
        rc = 0;
    }
#if 0
    // XXX this code is not tenyr-correct -- it can block
    else if (op == OP_DATA_READ) {
        int tmp;
        if ((*data = tmp = getchar()) && tmp == EOF) {
            return -1;
        }
    }
#endif

    return rc;
}

int serial_add_device(struct device *device)
{
    *device = (struct device){
        .bounds = { SERIAL_BASE, SERIAL_BASE + 1 },
        .ops = {
            .op = serial_op,
            .init = serial_init,
            .fini = serial_fini,
        },
    };

    return 0;
}

/* vi: set ts=4 sw=4 et: */
