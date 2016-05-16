#include "sim.h"

#include <vpi_user.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

struct dispatch_userdata {
    struct tenyr_sim_state *state;
    vpiHandle array;
};

static int vpi_dispatch(void *ud, int op, uint32_t addr, uint32_t *data)
{
    int rc = -1;
    struct dispatch_userdata *d = ud;

    vpiHandle word = vpi_handle_by_index(d->array, addr);
    struct t_vpi_value argval = { .format = vpiIntVal };
    switch (op) {
        case OP_WRITE:
            argval.value.integer = *data;
            vpi_put_value(word, &argval, NULL, vpiNoDelay);
            rc = 0;
            break;
        case OP_INSN_READ:
        case OP_DATA_READ:
            vpi_get_value(word, &argval);
            *data = argval.value.integer;
            rc = 0;
            break;
        // default case should be impossible -- leaves -1 in rc
    }

    return rc;
}

static int get_range(vpiHandle from, int which)
{
    vpiHandle lr = vpi_handle(which, from);
    struct t_vpi_value argval = { .format = vpiIntVal };
    vpi_get_value(lr, &argval);
    return argval.value.integer;
}

int tenyr_sim_load(struct tenyr_sim_state *state)
{
    int rc = 0;
    vpiHandle array = vpi_handle_by_name("Top.tenyr.ram.store", NULL);

    struct dispatch_userdata data = {
        .state = state,
        .array = array,
    };

    int left  = get_range(array, vpiLeftRange);
    int right = get_range(array, vpiRightRange);
    int min = MIN(left, right);

    vpiHandle systfref  = vpi_handle(vpiSysTfCall, NULL),
              args_iter = vpi_iterate(vpiArgument, systfref),
              argh      = vpi_scan(args_iter);

    struct t_vpi_value argval = { .format = vpiStringVal };
    vpi_get_value(argh, &argval);
    const char *filename = argval.value.str;

    vpi_free_object(args_iter);

    FILE *stream = fopen(filename, "rb");
    if (!stream)
        return 1;

    void *ud = NULL;
    const struct format *f = &tenyr_asm_formats[0];
    if (f->init(stream, NULL, &ud))
        return 1;

    rc |= load_sim(vpi_dispatch, &data, f, ud, stream, min);

    rc |= f->fini(stream, &ud);

    return rc;
}

