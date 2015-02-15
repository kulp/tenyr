#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h>

struct sim_state;

#include "common.h"
#include "plugin_portable.h"

typedef int map_init(struct plugin_cookie *pcookie, void *ud);
typedef int map_op(void *cookie, int op, uint32_t addr, uint32_t *data);
typedef int map_cycle(void *cookie);
typedef int map_fini(void *cookie);

struct device {
    uint32_t bounds[2]; // lower and upper memory bounds, inclusive
    struct device_ops {
        map_init *init;
        map_op *op;
        map_cycle *cycle;
        map_fini *fini;
    } ops;
    void *cookie;
};

#endif

/* vi: set ts=4 sw=4 et: */
