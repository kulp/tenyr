#include "tenyr_vpi.h"
#include <stdlib.h>

int tenyr_sim_load(struct tenyr_sim_state *state, void *userdata)
{
    vpiHandle ram = vpi_handle_by_name("Test.top.ram", NULL);
    vpiHandle array = vpi_handle_by_name("store", ram);
    vpiHandle word = vpi_handle_by_index(array, 4096);
    vpi_printf("word = %p\n", (void*)word);
    vpi_printf("\t%s\n", vpi_get_str(vpiName, word));
    vpi_printf("\t%s\n", vpi_get_str(vpiType, word));
    //vpiHandle bit = vpi_handle_multi(ram, 2, (int[]){ 0, 0 });
    //vpi_printf("bit = %p\n", (void*)bit);

    {
        vpiHandle ara, itr;
        itr = vpi_iterate(vpiMemory, ram);
        while ((ara = vpi_scan(itr))) {
            vpi_printf("\t%s\n", vpi_get_str(vpiName, ara));
            vpiHandle itr;
            itr = vpi_iterate(vpiMemoryWord, ara);
            while ((ara = vpi_scan(itr))) {
                vpi_printf("\t%s\n", vpi_get_str(vpiName, ara));
            }
        }
    }

    return 0;
}

