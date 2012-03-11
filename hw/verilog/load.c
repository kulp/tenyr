#include "tenyr_vpi.h"
#include "asm.h"
#include "sim.h"
#include "ops.h"

#include <stdlib.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

struct dispatch_userdata {
    struct tenyr_sim_state *state;
    vpiHandle array;
};

static int dispatch_op(void *ud, int op, uint32_t addr, uint32_t *data)
{
    struct dispatch_userdata *d = ud;
    struct tenyr_sim_state *s = d->state;

    vpiHandle word = vpi_handle_by_index(d->array, addr);
    struct t_vpi_value argval = { .format = vpiIntVal };
    switch (op) {
        case OP_WRITE:
            argval.value.integer = *data;
            vpi_put_value(word, &argval, NULL, vpiNoDelay);
            if (s->debug > 4)
                vpi_printf("put value %#x @ %#x\n", *data, addr);
            break;
        case OP_READ:
            vpi_get_value(word, &argval);
            *data = argval.value.integer;
            if (s->debug > 4)
                vpi_printf("got value %#x @ %#x\n", *data, addr);
            break;
        default:
            fatal(0, "Invalid op type %d", op);
    }

    return 0;
}

static int get_range(vpiHandle from, int which)
{
    vpiHandle lr = vpi_handle(which, from);
    struct t_vpi_value argval = { .format = vpiIntVal };
    vpi_get_value(lr, &argval);
    return argval.value.integer;
}

int tenyr_sim_load(struct tenyr_sim_state *state, void *userdata)
{
    vpiHandle ram = vpi_handle_by_name("Test.top.ram", NULL);
    vpiHandle array = vpi_handle_by_name("store", ram);

    struct dispatch_userdata data = {
        .state = state,
        .array = array,
    };

    int left  = get_range(array, vpiLeftRange);
    int right = get_range(array, vpiRightRange);
    int min = MIN(left, right);

    load_sim(dispatch_op, &data, &formats[0], stdin, min);

    return 0;
}

