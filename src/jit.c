#include "ops.h"
#include "common.h"

#include "jit.h"
#include "lightning.h"

// GNU Lightning's macros expect `_jit` to be a jit_state_t pointer
#define _jit (*(jit_state_t**)(&s->jj))

void jit_init(struct jit_state **pstate)
{
    init_jit(NULL);
    struct jit_state *s = *pstate = calloc(1, sizeof *s);
}

void jit_fini(struct jit_state *s)
{
    jit_destroy_state();
    finish_jit();
}

Block *jit_gen_block(void *cookie, int len, int32_t *instructions)
{
    struct jit_state *s = cookie;

    s->jj = jit_new_state();

    jit_prolog();
    // TODO
    jit_ret();
    jit_epilog();

    Block *result = jit_emit();
    jit_clear_state();

    return result;
}
