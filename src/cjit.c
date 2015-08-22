#define _GNU_SOURCE 1 /* for RTLD_DEFAULT */
#include "jit.h"

#include <assert.h>
#include <dlfcn.h>
#include <search.h>

// XXX permit fetch to express failure
int32_t fetch(void *cookie, int32_t addr)
{
    struct sim_state *s = cookie;
    int32_t data;
    s->dispatch_op(s, OP_DATA_READ, addr, (uint32_t*)&data);
    return data;
}

void store(void *cookie, int32_t addr, int32_t value)
{
    struct sim_state *s = cookie;
    s->dispatch_op(s, OP_WRITE, addr, (uint32_t*)&value);
}

static int bb_by_base(const void *a_, const void *b_)
{
    const struct basic_block *a = a_, *b = b_;

    return a->base - b->base;
}

static int pre_insn_hook(struct sim_state *s, const struct element *i, void *ud)
{
    struct ops_state *o = (struct ops_state*)ud;
    if (!o->curr_bb) {
        struct basic_block nb = { 0, 0, i->insn.reladdr, 0, NULL, NULL };
        struct basic_block **f = tsearch(&nb, &o->basic_blocks, bb_by_base);
        if (*f == &nb) {
            *f = malloc(sizeof **f);
            **f = nb;
        }
        o->curr_bb = *f;
    }

    struct basic_block *bb = o->curr_bb;
    if (bb->compiled)
        return 1; // indicate to jit_run_sim that JIT should be used

    if (bb->run_count == o->js->run_count_threshold) {
        // Cache the instructions we receive so they are ready for compilation
        if (!bb->cache)
            bb->cache = calloc(bb->len, sizeof *bb->cache);
        bb->cache[(uint32_t)i->insn.reladdr - (uint32_t)bb->base] = i->insn.u.word;
    } else if (bb->run_count > o->js->run_count_threshold) {
        assert(bb->complete);
        bb->compiled = jit_gen_block(o->js, bb->len, bb->cache);
        free(bb->cache);
        return 1; // indicate to jit_run_sim that JIT should be used
    }

    return o->ops.pre_insn(s, i, o->nested_ops_data);
}

static int post_insn_hook(struct sim_state *s, const struct element *i, void *ud)
{
    struct ops_state *o = (struct ops_state*)ud;

    int dd = i->insn.u.typeany.dd;
    if ((dd == 0 || dd == 3) && i->insn.u.typeany.z == 0xf) { // P is being updated
        struct basic_block *bb = o->curr_bb;
        bb->len = (uint32_t)i->insn.reladdr - (uint32_t)bb->base + 1;
        bb->complete = 1;
        bb->run_count++;
        o->curr_bb = NULL;
        o->ops.post_insn(s, i, o->nested_ops_data); // allow hook to run one last time
        return 1;
    }

    return o->ops.post_insn(s, i, o->nested_ops_data);
}

int jit_run_sim(struct sim_state *s, struct run_ops *ops, void **run_data, void *ops_data)
{
    struct jit_state *js;
    jit_init(&js);
    *run_data = js;
    js->sim_state = s;

    const void *val = NULL;
    if (param_get(s->conf.params, "tsim.jit.run_count_threshold", 1, &val)) {
        char *next = NULL;
        if (val) {
            int v = strtol((char*)val, &next, 0);
            if (next != val && next != NULL && *next == '\0' && v > 1)
                js->run_count_threshold = v;
        }
    }

    struct ops_state ops_state = { 0 }, *o = &ops_state;
    o->js = js;
    o->ops.pre_insn = ops->pre_insn;
    o->ops.post_insn = ops->post_insn;
    o->nested_ops_data = ops_data;

    struct run_ops wrappers = { 0 };
    wrappers.pre_insn = pre_insn_hook;
    wrappers.post_insn = post_insn_hook;

    typedef int dispatch_cycle(struct sim_state *s);
    void *pv = dlsym(RTLD_DEFAULT, "devices_dispatch_cycle");
    void *iv = dlsym(RTLD_DEFAULT, "interp_run_sim");
    dispatch_cycle *pump   = ALIASING_CAST(dispatch_cycle, pv);
    sim_runner     *interp = ALIASING_CAST(sim_runner, iv);
    if (!interp || !pump)
        fatal(PRINT_ERRNO, "Failed to find interpreter");

    int rc = 0;
    do {
        if (o->curr_bb && o->curr_bb->compiled) {
            o->curr_bb->compiled(s, s->machine.regs);
            s->insns_executed += o->curr_bb->len;
            o->curr_bb->run_count++;
            o->curr_bb = NULL;
        }
        rc = interp(s, &wrappers, &js->nested_run_data, o);
        pump(s); // no longer a "cycle" but periodic
    } while (rc >= 0);

    jit_fini(js);
    return 0;
}
