#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "device.h"
#include "sim.h"
#include "os_common.h"

#define SERIAL_BASE (1ULL << 5)
#define SERIAL_NO_CHARACTER 0x80000000ull

device_adder serial_add_device;

struct serial_state {
    FILE *in;
    FILE *out;
};

static int serial_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    int rc = 0;
    // Assume that serial_init will not be called more than once per serial_fini
    struct serial_state *s = *(void**)cookie = malloc(sizeof *s);
    s->in  = stdin;
    s->out = stdout;

    // TODO make it possible to select different streams

    // By default, we don't set non-blocking on the input stream, even though
    // that would be the best behaviour. If we set non-blocking on stdin, we
    // could affect behaviour even after tsim exits. The default behaviour,
    // blocking on input from stdin, results in simulation pausing until data
    // is available. This is not realistic, but represents an acceptable
    // compromise for now, without adding significant complexity to add
    // correct cross-platform non-blocking behaviour. User (i.e. tenyr
    // assembly) code should always be written to check for
    // SERIAL_NO_CHARACTER.
    int nb;
    if (pcookie->gops.param_get_int(pcookie, "tsim.serial.set_non_blocking", &nb) && nb)
        rc = os_set_non_blocking(s->in);

    return rc;
}

static int serial_fini(void *cookie)
{
    free(cookie);
    // TODO close streams if they were not standard ones
    return 0;
}

static int serial_op(void *cookie, int op, int32_t addr, int32_t *data)
{
    struct serial_state *s = cookie;
    int rc = -1;

    if (op == OP_WRITE) {
        // Use a temporary variable to get the semantically least significant
        // bits of `data` into a character. Writing out one byte as pointed to
        // by `data` (as was once done here) improperly assumes endianness.
        const char ch = *data;
        fwrite(&ch, 1, 1, s->out);
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
