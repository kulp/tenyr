#ifndef TENYR_VPI_
#define TENYR_VPI_

struct tenyr_sim_state;

typedef int tenyr_sim_cb(struct tenyr_sim_state *state, void *userdata);

struct tenyr_sim_state {
    int debug;      ///< debugging level
    struct {
        tenyr_sim_cb *genesis;      ///< beginning of sim
        tenyr_sim_cb *posedge;      ///< rising  edge of clock
        tenyr_sim_cb *negedge;      ///< falling edge of clock
        tenyr_sim_cb *apocalypse;   ///< end of sim
    } cb;
    void *extstate; ///< external state possibly used by tenyr_sim_cb's
};

#endif

