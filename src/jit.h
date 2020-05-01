#ifndef JIT_H_
#define JIT_H_

#include "sim.h"
#include <stdint.h>

typedef void Block(struct sim_state *sim, int32_t *registers);

struct jit_state {
    void *jj;

    struct {
        int32_t (*fetch)(struct sim_state *s, int32_t addr);
        void (*store)(struct sim_state *s, int32_t addr, int32_t value);
    } ops;
    void *nested_run_data;
    struct sim_state *sim_state;
    int run_count_threshold;
};

struct ops_state {
    struct jit_state *js;
    struct run_ops ops;
    void *nested_ops_data;
    void *basic_blocks;
    struct basic_block *curr_bb;
};

struct basic_block {
    Block *compiled;
    int32_t *cache;
    int run_count;
    int32_t base;
    uint32_t len;
};

void jit_init(struct jit_state **state);
void jit_fini(struct jit_state *state);
Block *jit_gen_block(struct jit_state *s, int len, int32_t *instructions);

#endif
