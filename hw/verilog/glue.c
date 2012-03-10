#include <vpi_user.h>
#include <stdlib.h>

static void *user_data = NULL;
static void *pud = (void*)&user_data;

void register_genesis(void)
{
    extern int tenyr_sim_genesis(p_cb_data data);
    s_cb_data data = { cbStartOfSimulation, tenyr_sim_genesis, NULL, 0, 0, 0, pud };
    vpi_register_cb(&data);
}

void register_apocalypse(void)
{
    extern int tenyr_sim_apocalypse(p_cb_data data);
    s_cb_data data = { cbEndOfSimulation, tenyr_sim_apocalypse, NULL, 0, 0, 0, pud };
    vpi_register_cb(&data);
}

void (*vlog_startup_routines[])() = {
    register_genesis,
    register_apocalypse,

    NULL
};

