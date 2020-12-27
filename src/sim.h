#ifndef SIM_H_
#define SIM_H_

#include "ops.h"
#include "asm.h"
#include "plugin.h"
#include "device.h"
#include "param.h"
#include "device.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

enum memory_op { OP_INSN_READ=0, OP_DATA_READ=1, OP_WRITE=2 };

typedef int op_dispatcher(void *ud, int op, int32_t addr, int32_t *data);

struct run_ops;

struct machine_state {
    struct device_list *devices, **last_device;
    int32_t regs[16];
};

struct sim_state;

typedef int sim_runner(struct sim_state *s, const struct run_ops *ops, void **run_data, void *ops_data);
typedef int dispatch_cycle(struct sim_state *s);

struct sim_state {
    struct {
        int verbose;
        int run_defaults;   ///< whether to run default recipes
        int debugging;

        struct param_state *params;

        int32_t start_addr;
        int32_t load_addr;
        int32_t halt_addr;
        const struct format *fmt;
        char *tsim_path;
    } conf;

    struct {
        struct plugin {
            void *cookie;
            struct device_ops ops;
        } *impls;
    } *plugins;
    int plugins_loaded;
    struct plugin_cookie plugin_cookie;

    op_dispatcher *dispatch_op;

    struct recipe_book  *recipes_head;
    struct recipe_book **recipes_tail;

    struct machine_state machine;
    sim_runner *run_sim, *interp;
    dispatch_cycle *pump;
    unsigned long insns_executed;

    struct library_list *libs; // opaque linked list of open libraries to dispose
};

struct run_ops {
    int (*pre_fetch)(struct sim_state *s, void *ud); ///< before instruction fetch
    int (*pre_insn)(struct sim_state *s, const struct element *i, void *ud); ///< before instruction execute
    int (*post_insn)(struct sim_state *s, const struct element *i, void *ud); ///< after instruction execute
};

extern sim_runner interp_run_sim, interp_step_sim;
int load_sim(op_dispatcher *dispatch_op, void *sud, const struct format *f,
        void *fud, STREAM *in, int32_t load_address);

#define breakpoint(...) \
    fatal(0, __VA_ARGS__)

struct device * new_device(struct sim_state *s);
int dispatch_op(void *ud, int op, int32_t addr, int32_t *data);
int devices_setup(struct sim_state *s);
int devices_finalise(struct sim_state *s);
int devices_teardown(struct sim_state *s);
int devices_dispatch_cycle(struct sim_state *s);

#endif

/* vi: set ts=4 sw=4 et: */
