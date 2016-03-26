#include "tenyr_vpi.h"

#include "common.h"
#include <stdlib.h>

int tenyr_sim_genesis(p_cb_data data)
{
    struct tenyr_sim_state *d = (void*)data->user_data;

#if DEBUG
    d->debug = DEBUG;
#endif

    if (d->debug > 4)
        vpi_printf("%s ; userdata = %p\n", __func__, (void*)data->user_data);

    extern int tenyr_sim_clock(p_cb_data data);
    vpiHandle clock = vpi_handle_by_name("Top.tenyr.clk", NULL);
    s_cb_data cbd = {
        .reason    = cbValueChange,
        .cb_rtn    = tenyr_sim_clock,
        .obj       = clock,
        .time      = &(s_vpi_time ){ .type   = vpiSimTime },
        .value     = &(s_vpi_value){ .format = vpiIntVal  },
        .user_data = data->user_data,
    };

    vpi_register_cb(&cbd);

    int rc = 0;
    if ((rc = setjmp(errbuf)))
        exit(rc);

    if (d->cb.genesis)
        d->cb.genesis(d, data);

    return 0;
}

int tenyr_sim_apocalypse(p_cb_data data)
{
    struct tenyr_sim_state *d = (void*)data->user_data;
    if (d->debug > 4)
        vpi_printf("%s ; userdata = %p\n", __func__, (void*)data->user_data);

    if (d->cb.apocalypse)
        d->cb.apocalypse(d, data);

    return 0;
}

int tenyr_sim_clock(p_cb_data data)
{
    struct tenyr_sim_state *d = (void*)data->user_data;
    int clk = data->value->value.integer;

    if (d->debug > 5) {
        vpi_printf("clock val %d\n", clk);
        vpi_printf("vpiSimTime %10lld\n",
                ((long long)data->time->high << 32) + data->time->low);
    }

    if (clk) {
        if (d->cb.posedge)
            d->cb.posedge(d, &clk);
    } else {
        if (d->cb.negedge)
            d->cb.negedge(d, &clk);
    }

    return 0;
}

