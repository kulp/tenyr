#include <vpi_user.h>
#include <stdlib.h>

static void *user_data = NULL;
static void *pud = (void*)&user_data;

typedef int tenyr_sim_cb(p_cb_data data);
typedef int tenyr_sim_tf();

void register_genesis(void)
{
    extern tenyr_sim_cb tenyr_sim_genesis;
    s_cb_data data = { cbStartOfSimulation, tenyr_sim_genesis, NULL, 0, 0, 0, pud };
    vpi_register_cb(&data);
}

void register_apocalypse(void)
{
    extern tenyr_sim_cb tenyr_sim_apocalypse;
    s_cb_data data = { cbEndOfSimulation, tenyr_sim_apocalypse, NULL, 0, 0, 0, pud };
    vpi_register_cb(&data);
}

void register_serial(void)
{
    extern tenyr_sim_tf tenyr_sim_putchar, tenyr_sim_getchar;
    s_vpi_systf_data put = { vpiSysTask, 0, "$putchar", tenyr_sim_putchar, NULL, NULL, pud };
    vpi_register_systf(&put);
    s_vpi_systf_data get = { vpiSysTask, 0, "$getchar", tenyr_sim_getchar, NULL, NULL, pud };
    vpi_register_systf(&get);
}

void (*vlog_startup_routines[])() = {
    register_genesis,
    register_apocalypse,

    register_serial,

    NULL
};

