#include "sim.h"

struct device * new_device(struct sim_state *s)
{
    struct device_list *d = calloc(1, sizeof *d);
    d->next = NULL;
    struct device_list ***p = &s->machine.last_device;
    **p = d;
    *p = &d->next;
    return &d->device;
}

static int devices_does_match(const uint32_t *addr, const struct device *device)
{
    if (*addr <= device->bounds[1]) {
        if (*addr >= device->bounds[0]) {
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
    struct device *device = NULL;
    for (struct device_list *dl = s->machine.devices; dl && !device; dl = dl->next)
        if (devices_does_match(&addr, &dl->device) == 0)
            device = &dl->device;

    int result = -1;
    if (device) {
        // TODO don't send in the whole simulator state ? the op should have
        // access to some state, in order to redispatch and potentially use
        // other machine.devices, but it shouldn't see the whole state
        result = device->ops.op(device->cookie, op, addr, data);

        if (s->conf.verbose > 2) {
            printf("%-5s @ 0x%08x = 0x%08x\n",
                    (op == OP_WRITE) ? "write" : "read", addr, *data);
        }

    } else if (s->conf.flags & SIM_CONTINUE_ON_INVALID_DEVICE) {
        // Unavailable memory returns all ones by architectural decision
        // TODO document this in the machine model
        if (op != OP_WRITE)
            *data = 0xffffffff;

        if (s->conf.verbose > 2) {
            printf("%-5s @ 0x%08x %s due to invalid device\n",
                    (op == OP_WRITE) ? "write" : "read", addr,
                    (op == OP_WRITE) ? "silently ignored" : "returning 0xffffffff");
        }
    } else {
        fprintf(stderr, "No device handles address %#x\n", addr);
        return -1;
    }

    return result;
}

int devices_setup(struct sim_state *s)
{
    s->machine.devices = NULL;
    s->machine.last_device = &s->machine.devices;

    return 0;
}

int devices_finalise(struct sim_state *s)
{
    for (struct device_list *d = s->machine.devices; d; d = d->next)
        d->device.ops.init(&s->plugin_cookie, &d->device, &d->device.cookie);

    return 0;
}

int devices_teardown(struct sim_state *s)
{
    for (struct device_list *d = s->machine.devices, *e; d; d = e) {
        e = d->next;
        d->device.ops.fini(d->device.cookie);
        free(d);
    }

    return 0;
}

int devices_dispatch_cycle(struct sim_state *s)
{
    int rc = 0;
    for (struct device_list *d = s->machine.devices; d; d = d->next)
        if (d->device.ops.cycle)
            rc = d->device.ops.cycle(d->device.cookie);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
