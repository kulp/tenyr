#include "emscripten.h"
#include "sim.h"

// In lieu of a header, provide a prototype for -Wmissing-prototypes.
int recipe_emscript(struct sim_state *s);

struct emscripten_wrap {
    struct sim_state *s;
    const struct run_ops *ops;
    void **run_data;
    void *ops_data;
    int insns_per_frame;
    int rc;
};

static void emscripten_wrap_step(void *data_)
{
    struct emscripten_wrap *d = data_;
    int runs = 0;
    do {
        d->rc = interp_step_sim(d->s, d->ops, d->run_data, d->ops_data);
    } while (d->rc > 0 && runs++ < d->insns_per_frame);

    // When interp_step_sim returns <= 0 the simulation has stopped (0 = a hook
    // told us to stop; -1 = error / illegal instruction / halt).  Cancel the
    // event loop so the browser doesn't busy-spin.
    if (d->rc <= 0) {
        emscripten_cancel_main_loop();
        free(d);
    }
}

static int emscripten_setup(struct sim_state *s, const struct run_ops *ops,
        void **run_data, void *ops_data)
{
    // Heap-allocate the wrap context: emscripten_set_main_loop_arg defers the
    // callback to the event loop, so a stack-allocated struct would be dangling
    // by the time it fires.
    struct emscripten_wrap *data = malloc(sizeof *data);
    *data = (struct emscripten_wrap){
        .s = s,
        .ops = ops,
        .run_data = run_data,
        .ops_data = ops_data,
        .insns_per_frame = 1,
        .rc = 0,
    };

    param_get_int(s->conf.params, "emscripten.insns_per_anim_frame", &data->insns_per_frame);
    emscripten_cancel_main_loop(); // In case we get called more than once
    emscripten_set_main_loop_arg(emscripten_wrap_step, data, 0, 1);

    return data->rc;
}

int recipe_emscript(struct sim_state *s)
{
    s->run_sim = emscripten_setup;
    return 0;
}

/* vi: set ts=4 sw=4 et: */
