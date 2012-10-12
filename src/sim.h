#ifndef SIM_H_
#define SIM_H_

#include "ops.h"
#include "machine.h"
#include "asm.h"
#include "plugin.h"
#include "device.h"
#include "param.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

struct sim_state;

typedef int recipe(struct sim_state *s);

enum memory_op { OP_READ=0, OP_WRITE=1 };

struct recipe_book {
    recipe *recipe;
    struct recipe_book *next;
};

typedef int op_dispatcher(void *ud, int op, uint32_t addr, uint32_t *data);

struct sim_state {
    struct {
        int abort;
        int nowrap;
        int verbose;
        int run_defaults;   ///< whether to run default recipes
        int debugging;

        struct param_state params;

        int start_addr;
        int load_addr;
        const struct format *fmt;
    } conf;

    struct {
        struct plugin {
            void *cookie;
            struct device_ops ops;
        } *impls;
    } *plugins;
    struct plugin_cookie plugin_cookie;

    op_dispatcher *dispatch_op;

    struct recipe_book *recipes;

    struct machine_state machine;
};

struct run_ops {
    int (*pre_insn)(struct sim_state *s, struct instruction *i);
    int (*post_insn)(struct sim_state *s, struct instruction *i);
};

int run_instruction(struct sim_state *s, struct instruction *i);
int run_sim(struct sim_state *s, struct run_ops *ops);
int load_sim(op_dispatcher *dispatch_op, void *sud, const struct format *f,
        FILE *in, int load_address);

// TODO convert this to an interrupt in the debugger
#define breakpoint(...) \
    fatal(0, __VA_ARGS__)

#endif

