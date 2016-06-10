#include "common.h"

struct sim_state;

int recipe_emscript(struct sim_state *s)
{
    (void)s;
    fatal(0, "emscripten recipe not applicable in this build");
    return -1;
}

