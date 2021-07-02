#include "sim.h"
#include "stream.h"

#include "tenyr_vpi.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

struct dispatch_userdata {
    struct tenyr_sim_state *state;
    vpiHandle array;
};

static int vpi_dispatch(void *ud, int op, int32_t addr, int32_t *data)
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

PLI_INT32 tenyr_sim_load(PLI_BYTE8 *userdata)
{
    struct tenyr_sim_state *state = (void*)userdata;
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

    int rc = 0;
    do {
        struct t_vpi_value argval = { .format = vpiStringVal };
        vpi_get_value(argh, &argval);
        vpi_free_object(argh);
        const char *filename = argval.value.str;

        FILE *file = fopen(filename, "rb");
        rc = file == NULL;
        if (rc)
            break;

        STREAM stream_ = stream_make_from_file(file), *stream = &stream_;

        void *ud = NULL;
        const struct format *f = &tenyr_asm_formats[0];

        rc = f->init(stream, NULL, &ud);
        if (rc)
            break;

        rc |= load_sim(vpi_dispatch, &data, f, ud, stream, min);

        rc |= f->fini(stream, &ud);
    } while (0);

    vpiHandle second = vpi_scan(args_iter);
    if (second) { // The second argument might not be provided.
        s_vpi_value retval = {
            .format = vpiIntVal,
            .value.integer = rc,
        };

        vpi_put_value(second, &retval, NULL, vpiNoDelay);
        vpi_free_object(second);
    }

    return 0;
}

