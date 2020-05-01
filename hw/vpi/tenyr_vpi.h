#ifndef TENYR_VPI_
#define TENYR_VPI_

#include <vpi_user.h>

struct tenyr_sim_state;

typedef int tenyr_sim_cb(p_cb_data data);
typedef int tenyr_sim_call(struct tenyr_sim_state *state, void *userdata);

struct tenyr_sim_state {
    struct {
        tenyr_sim_call *genesis;      ///< beginning of sim
        tenyr_sim_call *posedge;      ///< rising  edge of clock
        tenyr_sim_call *negedge;      ///< falling edge of clock
        tenyr_sim_call *apocalypse;   ///< end of sim
    } cb;
    struct {
        struct {
            vpiHandle genesis, apocalypse;
        } cb;
        struct {
            vpiHandle tenyr_load;
            vpiHandle tenyr_putchar;
#if 0
            // XXX this code is not tenyr-correct -- it can block
            vpiHandle tenyr_getchar;
#endif
        } tf;
    } handle;
    void *extstate; ///< external state possibly used by tenyr_sim_cb's
};

extern tenyr_sim_cb tenyr_sim_genesis;
extern tenyr_sim_cb tenyr_sim_apocalypse;
extern tenyr_sim_cb tenyr_sim_clock;

extern int tenyr_sim_load(struct tenyr_sim_state *state);
extern int tenyr_sim_putchar(struct tenyr_sim_state *state);
extern int tenyr_sim_getchar(struct tenyr_sim_state *state);

#endif

