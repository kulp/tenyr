#ifndef SIM_H_
#define SIM_H_

struct state {
    struct {
        int abort;
    } conf;

    size_t devices_count;
    struct device *devices;
    int (*dispatch_op)(struct state *s, int op, uint32_t addr, uint32_t *data);

    int32_t regs[16];
};

#endif

