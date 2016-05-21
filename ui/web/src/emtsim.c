#include "plugin.h"

#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"

#include <stdlib.h>

static int pre_insn(struct sim_state *s, const struct element *i, void *ud)
{
    (void)s;
    (void)i;
    (void)ud;
    return -1;
}

static int post_insn(struct sim_state *s, const struct element *i, void *ud)
{
    (void)s;
    (void)i;
    (void)ud;
    return -1;
}

static void em_device_setup(struct sim_state *s)
{
    devices_setup(s);
    {
        extern int serial_add_device(struct device *device);
        serial_add_device(&new_device(s)->device);
    }

    {
        extern int ram_add_device(struct device *device);
        struct device_list *dl = new_device(s);
        ram_add_device(&dl->device);
        dev->ops.init(&s->plugin_cookie, &dl->device.cookie);
    }

    {
        extern int ram_add_device(struct device *device);
        struct device *dl = new_device(s);
        ram_add_device(&dl->device);
        dev->ops.init(&s->plugin_cookie, &dl->device.cookie);
    }
    devices_finalise(s);
}

int main(void)
{
    int rc = 0;

    FILE *in = stdin;

    struct sim_state _s = {
        .conf = {
            .verbose      = 0,
            .run_defaults = 1,
            .debugging    = 0,
            .start_addr   = RAM_BASE,
            .load_addr    = RAM_BASE,
            .fmt          = &tenyr_asm_formats[2], // text
        },
        .dispatch_op = dispatch_op,
        .plugin_cookie = {
            .gops         = {
                .fatal = fatal_,
                .debug = debug_,
                .param_get = NULL, // TODO
                .param_set = NULL, // TODO
            },
        },
    }, *s = &_s;

    param_init(&s->conf.params);
    s->plugin_cookie.param = s->conf.params;

    em_device_setup(s);

    // This clearerr() is necessary to allow multiple runs of main() in
    // emscripten
    clearerr(in);
    void *ud = NULL;
    const struct format *f = s->conf.fmt;
    if (f->init(in, s->conf.params, &ud))
        fatal(0, "Error during initialisation for format '%s'", f->name);

    if (load_sim(s->dispatch_op, s, f, ud, in, s->conf.load_addr))
        fatal(0, "Error while loading state into simulation");

    f->fini(in, &ud);

    s->machine.regs[15] = s->conf.start_addr;

    struct run_ops ops = {
        .pre_insn = pre_insn,
        .post_insn = post_insn,
    };

    void *run_ud = NULL;
    interp_run_sim(s, &ops, &run_ud, NULL);

    //param_destroy(&s->conf.params);

    return rc;
}

