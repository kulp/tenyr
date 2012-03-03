#ifndef SIM_H_
#define SIM_H_

#include "ops.h"
#include "machine.h"

#include <stdint.h>
#include <stddef.h>

struct state;

typedef int recipe(struct state *s);

enum memory_op { OP_READ=0, OP_WRITE=1 };

struct recipe_book {
    recipe *recipe;
    struct recipe_book *next;
};

// TODO rename to sim_state or some such
struct state {
    struct {
        int abort;
        int nowrap;
        int verbose;
        int run_defaults;   ///< whether to run default recipes
        int debugging;
    } conf;

    int (*dispatch_op)(struct state *s, int op, uint32_t addr, uint32_t *data);

    struct recipe_book *recipes;

    struct mstate machine;
};

int run_instruction(struct state *s, struct instruction *i);

#endif

