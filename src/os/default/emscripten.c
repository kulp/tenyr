#include "common.h"

struct sim_state;

// In lieu of a header, provide a prototype for -Wmissing-prototypes.
int recipe_emscript(struct sim_state *s);

int recipe_emscript(struct sim_state *s)
{
    (void)s;
    fatal(0, "emscripten recipe not applicable in this build");
    return -1;
}

