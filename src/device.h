#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdint.h>

#include "common.h"
#include "plugin_portable.h"

struct device;

typedef int map_init(struct plugin_cookie *pcookie, struct device *device, void *ud);
typedef int map_op(void *cookie, int op, uint32_t addr, uint32_t *data);
typedef int map_cycle(void *cookie);
typedef int map_fini(void *cookie);

struct device_ops {
    map_init *init;
    map_op *op;
    map_cycle *cycle;
    map_fini *fini;
};

struct device {
    uint32_t bounds[2]; // lower and upper memory bounds, inclusive
    struct device_ops ops;
    void *cookie;
};

struct device_list {
    struct device device;
    struct device_list *next;
};

#endif

/* vi: set ts=4 sw=4 et: */
