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

static void build_insn(struct jit_state *s, int32_t insn, int offset)
{
    int a = -1, b = -1, c = -1;

    union insn u = { .word = insn };
    struct instruction_type012 t = u.type012;
    struct instruction_type3   v = u.type3;
    int op = t.op;
    // TODO
}

Block *jit_gen_block(void *cookie, int len, int32_t *instructions)
{
    struct jit_state *s = cookie;

    s->jj = jit_new_state();

    jit_prolog();
    jit_frame(3 * 8);
    jit_node_t *ss = jit_arg();
    jit_getarg(JIT_V0, ss);
    jit_node_t *rr = jit_arg();
    jit_getarg(JIT_V1, rr);

    for (int i = 0; i < len; i++)
        build_insn(s, instructions[i], i);

    jit_ret();
    jit_epilog();

    Block *result = jit_emit();
    jit_clear_state();

    return result;
}
