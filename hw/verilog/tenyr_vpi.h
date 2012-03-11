#ifndef TENYR_VPI_
#define TENYR_VPI_

#include <vpi_user.h>

struct tenyr_sim_state;

typedef int tenyr_sim_cb(p_cb_data data);
typedef int tenyr_sim_call(struct tenyr_sim_state *state, void *userdata);
typedef int tenyr_sim_tf();

struct tenyr_sim_state {
    int debug;      ///< debugging level
    struct {
        tenyr_sim_call *genesis;      ///< beginning of sim
        tenyr_sim_call *posedge;      ///< rising  edge of clock
        tenyr_sim_call *negedge;      ///< falling edge of clock
        tenyr_sim_call *apocalypse;   ///< end of sim
    } cb;
    void *extstate; ///< external state possibly used by tenyr_sim_cb's
};

int tenyr_sim_load(struct tenyr_sim_state *state, void *userdata);

#endif

