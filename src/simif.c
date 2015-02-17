#include "sim.h"

#include <assert.h>

int next_device(struct sim_state *s)
{
    while (s->machine.devices_count >= s->machine.devices_max) {
        s->machine.devices_max *= 2;
        s->machine.devices = realloc(s->machine.devices,
                s->machine.devices_max * sizeof *s->machine.devices);
    }

    return s->machine.devices_count++;
}

static int find_device_by_addr(const void *_test, const void *_in)
{
    const uint32_t *addr = _test;
    const struct device * const *in = _in;

    if (!*in)
        return 0;

    if (*addr <= (*in)->bounds[1]) {
        if (*addr >= (*in)->bounds[0]) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return 1;
    }
}

int dispatch_op(void *ud, int op, uint32_t addr, uint32_t *data)
{
    struct sim_state *s = ud;
    size_t count = s->machine.devices_count;
    struct device **device = bsearch(&addr, s->machine.devices, count, sizeof *device,
            find_device_by_addr);
    if (device == NULL || *device == NULL) {
        fprintf(stderr, "No device handles address %#x\n", addr);
        return -1;
    }
    // TODO don't send in the whole simulator state ? the op should have
    // access to some state, in order to redispatch and potentially use other
    // machine.devices, but it shouldn't see the whole state
    return (*device)->ops.op((*device)->cookie, op, addr, data);
}

static int compare_devices_by_base(const void *_a, const void *_b)
{
    const struct device * const *a = _a;
    const struct device * const *b = _b;

    assert(("LHS of device comparison is not NULL", *a != NULL));
    assert(("RHS of device comparison is not NULL", *b != NULL));

    return (*a)->bounds[0] - (*b)->bounds[0];
}

int devices_setup(struct sim_state *s)
{
    s->machine.devices_count = 0;
    s->machine.devices_max = 8;
    s->machine.devices = malloc(s->machine.devices_max * sizeof *s->machine.devices);

    return 0;
}

int devices_finalise(struct sim_state *s)
{
    if (s->conf.verbose > 2) {
        assert(("device to be wrapped is not NULL", s->machine.devices[0] != NULL));
        int debugwrap_wrap_device(struct device **device);
        debugwrap_wrap_device(&s->machine.devices[0]);
    }

    // Devices must be in address order to allow later bsearch. Assume they do
    // not overlap (overlap is illegal).
    qsort(s->machine.devices, s->machine.devices_count,
            sizeof *s->machine.devices, compare_devices_by_base);

    for (unsigned i = 0; i < s->machine.devices_count; i++)
        if (s->machine.devices[i])
            s->machine.devices[i]->ops.init(&s->plugin_cookie, &s->machine.devices[i]->cookie);

    return 0;
}

int devices_teardown(struct sim_state *s)
{
    for (unsigned i = 0; i < s->machine.devices_count; i++) {
        s->machine.devices[i]->ops.fini(s->machine.devices[i]->cookie);
        free(s->machine.devices[i]);
    }

    free(s->machine.devices);

    return 0;
}

int devices_dispatch_cycle(struct sim_state *s)
{
    for (size_t i = 0; i < s->machine.devices_count; i++)
        if (s->machine.devices[i]->ops.cycle)
            if (s->machine.devices[i]->ops.cycle(s->machine.devices[i]->cookie))
                return 1;

    return 0;
}

/* vi: set ts=4 sw=4 et: */
