#ifndef DEVICES_H_
#define DEVICES_H_

#include <stdint.h>

#include "common.h"

typedef int map_init(void *cookie, ...);
typedef int map_op(void *cookie, int op, uint32_t addr, uint32_t *data);
typedef int map_fini(void *cookie, ...);

struct device {
    uint32_t bounds[2]; // lower and upper memory bounds, inclusive
    map_init *init;
    map_op *op;
    map_fini *fini;
    void *cookie;
};

#endif

