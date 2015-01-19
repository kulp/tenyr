#include "sim.h"
#include "common.h"

#include <assert.h>
#include <stdlib.h>

static void do_op(enum op op, int type, int32_t *rhs, uint32_t X, uint32_t Y,
        uint32_t I)
{
    uint32_t p[3] = { 0 };
    // The `type` determines which position the immediate has
    switch (type) {
        case 0:
        case 1:
        case 2:
            p[type] = SEXTEND32(SMALL_IMMEDIATE_BITWIDTH, I);
            p[2 - (type > 1)] = X;
            p[1 - (type > 0)] = Y;
            break;
        case 3:
            p[0] = SEXTEND32(MEDIUM_IMMEDIATE_BITWIDTH, I);
            p[1] = p[2] = 0;
            break;
    }

    #define Ps(x) ((( int32_t*)p)[x])
    #define Pu(x) (((uint32_t*)p)[x])
    uint32_t pack2 = Pu(2) << 12;
    uint32_t pack1 = Pu(1) & ~(-1 << 12);
     int32_t pack0 = Ps(0);

    switch (op) {
        case OP_ADD               : *rhs =  (Ps(2) +  Ps(1)) + Ps(0); break;
        case OP_SUBTRACT          : *rhs =  (Ps(2) -  Ps(1)) + Ps(0); break;
        case OP_MULTIPLY          : *rhs =  (Ps(2) *  Ps(1)) + Ps(0); break;

        case OP_SHIFT_RIGHT_LOGIC : *rhs =  (Pu(2) >> Pu(1)) + Ps(0); break;
        case OP_SHIFT_RIGHT_ARITH : *rhs =  (Ps(2) >> Pu(1)) + Ps(0); break;
        case OP_SHIFT_LEFT        : *rhs =  (Pu(2) << Pu(1)) + Ps(0);
                                    if (Pu(1) >= 32) *rhs = 0;        break;

        case OP_COMPARE_LT        : *rhs = -(Ps(2) <  Ps(1)) + Ps(0); break;
        case OP_COMPARE_EQ        : *rhs = -(Ps(2) == Ps(1)) + Ps(0); break;
        case OP_COMPARE_GE        : *rhs = -(Ps(2) >= Ps(1)) + Ps(0); break;
        case OP_COMPARE_NE        : *rhs = -(Ps(2) != Ps(1)) + Ps(0); break;

        case OP_BITWISE_AND       : *rhs =  (Pu(2) &  Pu(1)) + Ps(0); break;
        case OP_BITWISE_ANDN      : *rhs =  (Pu(2) &~ Pu(1)) + Ps(0); break;
        case OP_BITWISE_OR        : *rhs =  (Pu(2) |  Pu(1)) + Ps(0); break;
        case OP_BITWISE_XOR       : *rhs =  (Pu(2) ^  Pu(1)) + Ps(0); break;
        case OP_BITWISE_XORN      : *rhs =  (Pu(2) ^~ Pu(1)) + Ps(0); break;

        case OP_PACK              : *rhs =  (pack2 |  pack1) + pack0; break;

        default:
            fatal(0, "Encountered illegal opcode %d", op);
    }
    #undef Ps
    #undef Pu
}

static int do_common(struct sim_state *s, int32_t *Z, int32_t *rhs,
        uint32_t *value, int deref_lhs, int deref_rhs, int reversed)
{
    int read_mem  = reversed ? deref_lhs : deref_rhs;
    int write_mem = reversed ? deref_rhs : deref_lhs;

    int32_t *r    = reversed ? Z         : rhs;
    int32_t *w    = reversed ? rhs       : Z;

    if (read_mem) {
        if (s->dispatch_op(s, OP_DATA_READ, *r, value)) {
            if (s->conf.abort) abort();
        }
    } else
        *value = *r;

    if (write_mem) {
        if (s->dispatch_op(s, OP_WRITE, *w, value)) {
            if (s->conf.abort) abort();
        }
    } else if (w != &s->machine.regs[0])  // throw away write to reg 0
        *w = *value;

    return 0;
}

int run_instruction(struct sim_state *s, struct element *i)
{
    int32_t *ip = &s->machine.regs[15];
    int32_t rhs = 0;
    uint32_t Y, imm, value;
    int op;

    ++*ip;

    struct instruction_type012 *g = &i->insn.u.type012;
    struct instruction_type3   *v = &i->insn.u.type3;

    switch (i->insn.u.typeany.p) {
        case 0:
        case 1:
        case 2:
            Y   = s->machine.regs[g->y];
            imm = g->imm;
            op  = g->op;
            break;
        case 3:
            Y   = 0;
            imm = v->imm;
            op  = OP_BITWISE_OR;
            break;
    }

    do_op(op, g->p, &rhs, s->machine.regs[g->x], Y, imm);
    do_common(s, &s->machine.regs[g->z], &rhs, &value, g->dd == 2,
            g->dd & 1, g->dd == 3);

    return 0;
}

int run_sim(struct sim_state *s, struct run_ops *ops)
{
    while (1) {
        if (s->machine.regs[15] == (signed)0xffffffff) { // end condition
            if (s->conf.abort) abort();
            return -1;
        }

        struct element i;
        if (s->dispatch_op(s, OP_INSN_READ, s->machine.regs[15], &i.insn.u.word)) {
            if (s->conf.abort) abort();
        }

        if (ops->pre_insn)
            ops->pre_insn(s, &i);

        if (run_instruction(s, &i))
            return 1;

        if (ops->post_insn)
            ops->post_insn(s, &i);
    }
}

int load_sim(op_dispatcher *dispatch, void *sud, const struct format *f,
        FILE *in, int load_address)
{
    void *ud = NULL;
    if (f->init)
        if (f->init(in, ASM_DISASSEMBLE, &ud))
            fatal(0, "Error during initialisation for format '%s'", f->name);

    struct element i;
    while (f->in(in, &i, ud) > 0) {
        // TODO stop assuming addresses are contiguous and monotonic
        if (dispatch(sud, OP_WRITE, load_address++, &i.insn.u.word))
            return -1;
    }

    if (f->fini)
        f->fini(in, &ud);

    return 0;
}

/* vi: set ts=4 sw=4 et: */
