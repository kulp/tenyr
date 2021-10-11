#include "tenyr_vpi.h"

static struct tenyr_sim_state tstate = { .cb.posedge = 0 }, *const pstate = &tstate;
static void *const pud = (void*)&tstate;

static int tenyr_sim_dummy(struct tenyr_sim_state *state, void *userdata)
{
    (void)state;
    (void)userdata;
    return 0;
}

static void register_genesis(void)
{
    s_cb_data data = { cbStartOfSimulation, tenyr_sim_genesis, NULL, 0, 0, 0, pud };
    pstate->handle.cb.genesis = vpi_register_cb(&data);
}

static void register_general(void)
{
    s_vpi_systf_data load = { vpiSysTask, 0, "$tenyr_load", tenyr_sim_load, NULL, NULL, pud };
    pstate->handle.tf.tenyr_load = vpi_register_systf(&load);

    pstate->cb.genesis = tenyr_sim_dummy;
    pstate->cb.posedge = tenyr_sim_dummy;
    pstate->cb.negedge = tenyr_sim_dummy;
    pstate->cb.apocalypse = tenyr_sim_dummy;
}

static void register_apocalypse(void)
{
    s_cb_data data = { cbEndOfSimulation, tenyr_sim_apocalypse, NULL, 0, 0, 0, pud };
    pstate->handle.cb.apocalypse = vpi_register_cb(&data);
}

static void register_serial(void)
{
    s_vpi_systf_data put = { vpiSysTask, 0, "$tenyr_putchar", tenyr_sim_putchar, NULL, NULL, pud };
    pstate->handle.tf.tenyr_putchar = vpi_register_systf(&put);
#if 0
    // XXX this code is not tenyr-correct -- it can block
    s_vpi_systf_data get = { vpiSysTask, 0, "$tenyr_getchar", tenyr_sim_getchar, NULL, NULL, pud };
    pstate->handle.tf.tenyr_getchar = vpi_register_systf(&get);
#endif
}

void (*vlog_startup_routines[])(void) = {
    register_genesis,
    register_general,
    register_apocalypse,

    register_serial,

    NULL
};

