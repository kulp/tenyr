#ifndef JIT_H_
#define JIT_H_

#include "sim.h"
#include <stdint.h>

typedef void Block(void *cookie, int32_t *registers);

struct jit_state {
    void *nested_run_data;
    void *sim_state;
    void *runtime; // actually an asmjit::JitRuntime
    int run_count_threshold;
    void *cc; // actually an asmjit::X86Compiler
};

struct ops_state {
    struct jit_state *js;
    struct run_ops ops;
    void *nested_ops_data;
    void *basic_blocks;
    struct basic_block *curr_bb;
};

struct basic_block {
    int run_count;
    unsigned complete;
    int32_t base;
    uint32_t len;
    Block *compiled;
    int32_t *cache;
};

#ifdef __cplusplus
# define EXTERN_C extern "C"
#else
# define EXTERN_C
#endif

EXTERN_C void jit_init(struct jit_state **state);
EXTERN_C void jit_fini(struct jit_state *state);

#endif
