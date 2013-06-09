#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/time.h>

#include "plugin.h"

#include "common.h"
#include "device.h"
#include "sim.h"

// TODO include EIB macros more generically
#include "../lib/irq.th"

struct eib_state {
    int dummy;
};

static int eib_init(struct plugin_cookie *pcookie, void *cookie, int nargs, ...)
{
    struct eib_state *state = *(void**)cookie;

    if (!state)
        state = *(void**)cookie = malloc(sizeof *state);

    *state = (struct eib_state){ .dummy = 0 };

    return 0;
}

static int eib_fini(void *cookie)
{
    struct eib_state *state = cookie;

    free(state);

    return 0;
}

static int eib_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct eib_state *state = cookie;

    return 0;
}

static int eib_pump(void *cookie)
{
    struct eib_state *state = cookie;

    return 0;
}

#if PLUGIN
void EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;
}
#endif

int EXPORT eib_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { TRAPJUMP, TRAP_ADDR },
        .ops = {
            .op = eib_op,
            .init = eib_init,
            .fini = eib_fini,
            .cycle = eib_pump,
        },
    };

    return 0;
}

