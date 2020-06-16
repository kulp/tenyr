#include "ops.h"
#include "common.h"

#include "jit.h"
#include "lightning.h"

// GNU Lightning's macros expect `_jit` to be a jit_state_t pointer
#define _jit (*(jit_state_t**)(&s->jj))

void jit_init(struct jit_state **pstate)
{
    init_jit(NULL);
    *pstate = calloc(1, sizeof **pstate);
}

void jit_fini(struct jit_state *s)
{
    if (s->jj != NULL)
        jit_destroy_state();
    finish_jit();
}

static void build_op(struct jit_state *s, int op, int result, int a, int b)
{
    switch (op & 0xf) {
        case OP_ADD:                jit_addr(result, a, b);     break;
        case OP_SUBTRACT:           jit_subr(result, a, b);     break;
        case OP_BITWISE_ANDN:       jit_comr(b, b);             /* FALLTHROUGH */
        case OP_BITWISE_AND:        jit_andr(result, a, b);     break;
        case OP_BITWISE_ORN:        jit_comr(b, b);             /* FALLTHROUGH */
        case OP_BITWISE_OR:         jit_orr(result, a, b);      break;
        case OP_BITWISE_XOR:        jit_xorr(result, a, b);     break;
        case OP_MULTIPLY:           jit_mulr(result, a, b);     break;

        case OP_SHIFT_RIGHT_LOGIC:  jit_rshr_u(result, a, b);   break;
        case OP_SHIFT_RIGHT_ARITH:  jit_rshr(result, a, b);     break;
        case OP_SHIFT_LEFT:         jit_lshr(result, a, b);     break;

        case OP_PACK:
            jit_lshi(a, a, 12);
            jit_andi(b, b, 0xfff);
            jit_orr(result, a, b);
            break;

        case OP_TEST_BIT:
            jit_movi(result, 1);
            jit_lshr(result, result, b);
            jit_andr(result, result, a);
            jit_nei(result, result, 0);
            jit_negr(result, result);
            break;

        case OP_COMPARE_LT:
            jit_ltr(result, a, b);
            jit_negr(result, result);
            break;

        case OP_COMPARE_EQ:
            jit_eqr(result, a, b);
            jit_negr(result, result);
            break;

        case OP_COMPARE_GE:
            jit_ger(result, a, b);
            jit_negr(result, result);
            break;
    }
}

static void build_insn(struct jit_state *s, insn in, int offset)
{
    const struct instruction_type012 t = in.type012;
    const struct instruction_type3   v = in.type3;

    const int x = JIT_R0;
    const int y = JIT_R1;
    const int i = JIT_R2;

    const int ss = JIT_V0;
    const int regs = JIT_V1;
    const int result = JIT_V2;

    if (t.x) {
        jit_ldxi_i(x, regs, t.x * 4);
        if (t.x == 15)
            jit_addi(x, x, offset + 1);
    } else {
        jit_movi(x, 0);
    }

    if (t.p == 3) {
        jit_addi(result, x, SEXTEND32(MEDIUM_IMMEDIATE_BITWIDTH,v.imm));
    } else {
        if (t.y) {
            jit_ldxi_i(y, regs, t.y * 4);
            if (t.y == 15)
                jit_addi(y, y, offset + 1);
        } else {
            jit_movi(y, 0);
        }
        jit_movi(i, SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,t.imm));

        int a = -1, b = -1, c = -1;
        switch (t.p) {
            case 0: a = x; b = y; c = i; break;
            case 1: a = x; b = i; c = y; break;
            case 2: a = i; b = x; c = y; break;
        }

        build_op(s, t.op, result, a, b);
        jit_addr(result, result, c);
    }

    const int tmp = JIT_R0; // reuse

    switch (t.dd) {
        case 0:
            if (t.z)
                jit_stxi_i(t.z * 4, regs, result);
            break;
        case 1:
            jit_ldxi_i(tmp, regs, t.z * 4);
            jit_prepare();
            jit_pushargr(ss);
            jit_pushargr(result);
            jit_pushargr(tmp);
            jit_finishi(s->ops.store);
            break;
        case 2:
            jit_ldxi_i(tmp, regs, t.z * 4);
            jit_prepare();
            jit_pushargr(ss);
            jit_pushargr(tmp);
            jit_pushargr(result);
            jit_finishi(s->ops.store);
            break;
        case 3:
            jit_prepare();
            jit_pushargr(ss);
            jit_pushargr(result);
            jit_finishi(s->ops.fetch);
            jit_retval_i(tmp);
            if (t.z)
                jit_stxi_i(t.z * 4, regs, tmp);
            break;
    }
}

Block *jit_gen_block(struct jit_state *s, int len, int32_t *instructions)
{
    s->jj = jit_new_state();

    jit_prolog();
    jit_frame(3 * 8);
    jit_node_t *ss = jit_arg();
    jit_getarg(JIT_V0, ss);
    jit_node_t *rr = jit_arg();
    jit_getarg(JIT_V1, rr);

    for (int i = 0; i < len; i++)
        build_insn(s, (insn){ .word = instructions[i] }, i);

    jit_ret();
    jit_epilog();

    Block *result = jit_emit();
    jit_clear_state();

    return result;
}
