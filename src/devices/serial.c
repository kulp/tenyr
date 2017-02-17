#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "os_common.h"

#define SERIAL_BASE (1ULL << 5)

struct serial_state {
    FILE *in;
    FILE *out;
};

static int serial_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    // Assume that serial_init will not be called more than once per serial_fini
    struct serial_state *s = *(void**)cookie = malloc(sizeof *s);
    s->in  = stdin;
    s->out = stdout;

    // TODO make it possible to select different streams
    return os_set_non_blocking(s->in);
}

static int serial_fini(void *cookie)
{
    free(cookie);
    // TODO close streams if they were not standard ones
    return 0;
}

static int serial_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct serial_state *s = cookie;
    int rc = -1;

    if (op == OP_WRITE) {
        fputc(*data, s->out);
        fflush(s->out);
        rc = ferror(s->out);
    } else if (op == OP_DATA_READ) {
        int tmp;
        if ((tmp = fgetc(s->in)) && tmp == EOF) {
            *data = SERIAL_NO_CHARACTER;
            rc = 0;
        } else {
            *data = tmp;
            rc = ferror(s->in);
        }
    }

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
