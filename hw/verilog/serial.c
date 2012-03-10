#include "tenyr_vpi.h"

#include <vpi_user.h>

int tenyr_sim_putchar(struct tenyr_sim_state *state)
{
    (void)state;

    vpiHandle systfref  = vpi_handle(vpiSysTfCall, NULL),
              args_iter = vpi_iterate(vpiArgument, systfref),
              argh      = vpi_scan(args_iter);

    struct t_vpi_value argval = { .format = vpiIntVal };
    argval.format = vpiIntVal;
    vpi_get_value(argh, &argval);
    putchar(argval.value.integer);

    vpi_free_object(args_iter);
    return 0;
}

int tenyr_sim_getchar(struct tenyr_sim_state *state)
{
    (void)state;

    vpiHandle systfref  = vpi_handle(vpiSysTfCall, NULL),
              args_iter = vpi_iterate(vpiArgument, systfref),
              argh      = vpi_scan(args_iter);

    struct t_vpi_value argval = {
        .format = vpiIntVal,
        .value.integer = getchar(),
    };
    vpi_put_value(argh, &argval, NULL, vpiNoDelay);

    vpi_free_object(args_iter);

    return 0;
}

