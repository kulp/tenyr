#include "plugin.h"

#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"

#include <stdlib.h>

#define EM_RAM_BASE (1ULL << 12)
#define EM_RAM_SIZE (4 * 1024)

int do_assembly(FILE *in, FILE *out, const struct format *f);

static int pre_insn(struct sim_state *s, struct element *i)
{
    (void)s;
    (void)i;
    return -1;
}

static int post_insn(struct sim_state *s, struct element *i)
{
    (void)s;
    (void)i;
    return -1;
}

static void em_device_setup(struct sim_state *s)
{
    devices_setup(s);
    {
        extern int serial_add_device(struct device **device);
        int index = next_device(s);
        struct device *dev = s->machine.devices[index] = malloc(sizeof *dev);
        serial_add_device(&dev);
    }

    {
        extern int ram_add_device(struct device **device);
        int index = next_device(s);
        struct device *dev = s->machine.devices[index] = malloc(sizeof *dev);
        ram_add_device(&dev);
        // manually adjust memory bounds
        dev->bounds[0] = EM_RAM_BASE;
        dev->bounds[1] = EM_RAM_BASE + EM_RAM_SIZE;
        dev->ops.init(&s->plugin_cookie, &dev->cookie, 2, EM_RAM_SIZE, dev->bounds[0]);
    }

    {
        extern int ram_add_device(struct device **device);
        int index = next_device(s);
        struct device *dev = s->machine.devices[index] = malloc(sizeof *dev);
        ram_add_device(&dev);
        // manually adjust memory bounds
        dev->bounds[0] = (1 << 24) - EM_RAM_SIZE;
        dev->bounds[1] = (1 << 24) - 1;
        dev->ops.init(&s->plugin_cookie, &dev->cookie, 2, EM_RAM_SIZE, dev->bounds[0]);
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
            .params = {
                .params_size  = DEFAULT_PARAMS_COUNT,
                .params_count = 0,
                .params       = calloc(DEFAULT_PARAMS_COUNT, sizeof *_s.conf.params.params),
            },
        },
        .dispatch_op = dispatch_op,
        .plugin_cookie = {
            .param = &_s.conf.params,
            .gops         = {
                .fatal = fatal_,
                .debug = debug_,
                .param_get = NULL, // TODO
                .param_set = NULL, // TODO
            },
        },
    }, *s = &_s;

    em_device_setup(s);

    if (load_sim(s->dispatch_op, s, s->conf.fmt, in, s->conf.load_addr))
        fatal(0, "Error while loading state into simulation");

    s->machine.regs[15] = s->conf.start_addr;

    struct run_ops ops = {
        .pre_insn = pre_insn,
        .post_insn = post_insn,
    };

    run_sim(s, &ops);

    if (in)
        fclose(in);

    //param_destroy(&s->conf.params);

    return rc;
}

