#include "device.h"
#include "plugin.h"

#include <stdint.h>

library_init tenyr_plugin_init;
device_adder failure_add_device;

struct failure_state;

#ifndef FAILURE_INIT
#define FAILURE_INIT 0
#endif

#ifndef FAILURE_FINI
#define FAILURE_FINI 0
#endif

#ifndef FAILURE_OP
#define FAILURE_OP 0
#endif

#ifndef FAILURE_PUMP
#define FAILURE_PUMP 0
#endif

#ifndef FAILURE_PLUGIN_INIT
#define FAILURE_PLUGIN_INIT 0
#endif

#ifndef FAILURE_ADD_DEVICE
#define FAILURE_ADD_DEVICE 0
#endif

#ifndef FAILURE_ADD_DEVICE_FUNC
#define FAILURE_ADD_DEVICE_FUNC failure_add_device
#endif

static int failure_init(struct plugin_cookie *pcookie, struct device *device, void *cookie)
{
    (void)pcookie;
    (void)device;
    (void)cookie;
    return FAILURE_INIT;
}

static int failure_fini(void *cookie)
{
    (void)cookie;
    return FAILURE_FINI;
}

static int failure_op(void *cookie, int op, int32_t addr, int32_t *data)
{
    (void)cookie;
    (void)op;
    (void)addr;
    (void)data;
    return FAILURE_OP;
}

static int failure_pump(void *cookie)
{
    (void)cookie;
    return FAILURE_PUMP;
}

int EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;

    return FAILURE_PLUGIN_INIT;
}

int EXPORT FAILURE_ADD_DEVICE_FUNC(struct device *device);
int EXPORT FAILURE_ADD_DEVICE_FUNC(struct device *device)
{
    *device = (struct device){
        .bounds = { INT32_MIN, INT32_MAX },
        .ops = {
            .op = failure_op,
            .init = failure_init,
            .fini = failure_fini,
            .cycle = failure_pump,
        },
    };

    return FAILURE_ADD_DEVICE;
}

/* vi: set ts=4 sw=4 et: */
