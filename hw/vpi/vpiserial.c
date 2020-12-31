#include "tenyr_vpi.h"

PLI_INT32 tenyr_sim_putchar(PLI_BYTE8 *userdata)
{
    struct tenyr_sim_state *state = (void*)userdata;
    (void)state;

    vpiHandle systfref  = vpi_handle(vpiSysTfCall, NULL),
              args_iter = vpi_iterate(vpiArgument, systfref),
              argh      = vpi_scan(args_iter);

    struct t_vpi_value argval = { .format = vpiIntVal };
    vpi_get_value(argh, &argval);
    putchar(argval.value.integer);

    vpi_free_object(args_iter);
    return 0;
}

#if 0
// XXX this code is not tenyr-correct -- it can block
PLI_INT32 tenyr_sim_getchar(PLI_BYTE8 *userdata)
{
    struct tenyr_sim_state *state = (void*)userdata;
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
#endif

