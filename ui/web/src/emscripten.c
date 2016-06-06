#include "emscripten.h"
#include "sim.h"

struct emscripten_wrap {
    struct sim_state *s;
    const struct run_ops *ops;
    void **run_data;
    void *ops_data;
    int rc;
};

static void emscripten_wrap_step(void *data_)
{
    struct emscripten_wrap *d = data_;
    d->rc = interp_run_sim_block(d->s, d->ops, d->run_data, d->ops_data);
}

static int emscripten_setup(struct sim_state *s, const struct run_ops *ops,
        void **run_data, void *ops_data)
{
    struct emscripten_wrap data = {
        .s = s,
        .ops = ops,
        .run_data = run_data,
        .ops_data = ops_data,
    };

    emscripten_cancel_main_loop(); // In case we get called more than once
    emscripten_set_main_loop_arg(emscripten_wrap_step, &data, 1000, 1);

    return data.rc;
}

int recipe_emscript(struct sim_state *s)
{
    s->run_sim = emscripten_setup;
    return 0;
}

