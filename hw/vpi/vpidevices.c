#include "tenyr_vpi.h"

#include <stdlib.h>

static struct tenyr_sim_state tstate = { 0 }, *pstate = &tstate;
static s_cb_data user_data = { .user_data = (void*)&tstate };
static void *pud = (void*)&user_data;

static void register_genesis(void)
{
    extern tenyr_sim_cb tenyr_sim_genesis;
    s_cb_data data = { cbStartOfSimulation, tenyr_sim_genesis, NULL, 0, 0, 0, pud };
    pstate->handle.cb.genesis = vpi_register_cb(&data);
}

static void register_general(void)
{
    extern tenyr_sim_tf tenyr_sim_load;
    s_vpi_systf_data load = { vpiSysTask, 0, "$tenyr_load", (int(*)())tenyr_sim_load, NULL, NULL, pud };
    pstate->handle.tf.tenyr_load = vpi_register_systf(&load);
}

static void register_apocalypse(void)
{
    extern tenyr_sim_cb tenyr_sim_apocalypse;
    s_cb_data data = { cbEndOfSimulation, tenyr_sim_apocalypse, NULL, 0, 0, 0, pud };
    pstate->handle.cb.apocalypse = vpi_register_cb(&data);
}

static void register_serial(void)
{
    extern tenyr_sim_tf tenyr_sim_putchar, tenyr_sim_getchar;
    s_vpi_systf_data put = { vpiSysTask, 0, "$tenyr_putchar", tenyr_sim_putchar, NULL, NULL, pud };
    pstate->handle.tf.tenyr_putchar = vpi_register_systf(&put);
    s_vpi_systf_data get = { vpiSysTask, 0, "$tenyr_getchar", tenyr_sim_getchar, NULL, NULL, pud };
    pstate->handle.tf.tenyr_getchar = vpi_register_systf(&get);
}

void (*vlog_startup_routines[])() = {
    register_genesis,
    register_general,
    register_apocalypse,

    register_serial,

    NULL
};

