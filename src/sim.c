#include "sim.h"
#include "common.h"

#include <stdlib.h>

static void do_op(enum op op, int type, int32_t *rhs, int32_t X, int32_t Y,
        int32_t I)
{
    int32_t p[3] = { 0 };
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
            p[1] = X;
            p[2] = 0;
            break;
    }

    #define Ps(x) ((( int32_t*)p)[x])
    #define Pu(x) (((uint32_t*)p)[x])
    #define Sh(Lv,Op,Ri) (int32_t)((Pu(Ri)) < 32 ? (Lv) Op (Pu(Ri)) : ((Lv) Op 31) Op 1)
    int32_t pack2 = (int32_t)(Pu(2) << 12);
    int32_t pack1 = (int32_t)(Pu(1) & 0xfff);
    int32_t pack0 = (int32_t)(Ps(0));
    int32_t mask  = (int32_t)((Pu(1) < 32) ? (1ull << Pu(1)) : 0);

    // This switch is exhaustive for the four bit opcode.
    switch (op & 0xf) {
        case OP_ADD               : *rhs =  (Ps(2) +  Ps(1)) + Ps(0); break;
        case OP_SUBTRACT          : *rhs =  (Ps(2) -  Ps(1)) + Ps(0); break;
        case OP_MULTIPLY          : *rhs =  (Ps(2) *  Ps(1)) + Ps(0); break;

        case OP_SHIFT_RIGHT_LOGIC : *rhs =  Sh(Pu(2), >>, 1) + Ps(0); break;
        case OP_SHIFT_RIGHT_ARITH : *rhs =  Sh(Ps(2), >>, 1) + Ps(0); break;
        case OP_SHIFT_LEFT        : *rhs =  Sh(Pu(2), <<, 1) + Ps(0); break;

        case OP_COMPARE_LT        : *rhs = -(Ps(2) <  Ps(1)) + Ps(0); break;
        case OP_COMPARE_EQ        : *rhs = -(Ps(2) == Ps(1)) + Ps(0); break;
        case OP_COMPARE_GE        : *rhs = -(Ps(2) >= Ps(1)) + Ps(0); break;

        case OP_BITWISE_AND       : *rhs =  (Ps(2) &  Ps(1)) + Ps(0); break;
        case OP_BITWISE_ANDN      : *rhs =  (Ps(2) &~ Ps(1)) + Ps(0); break;
        case OP_BITWISE_OR        : *rhs =  (Ps(2) |  Ps(1)) + Ps(0); break;
        case OP_BITWISE_ORN       : *rhs =  (Ps(2) |~ Ps(1)) + Ps(0); break;
        case OP_BITWISE_XOR       : *rhs =  (Ps(2) ^  Ps(1)) + Ps(0); break;

        case OP_PACK              : *rhs =  (pack2 |  pack1) + pack0; break;
        case OP_TEST_BIT          : *rhs = -!!(Ps(2) & mask) + Ps(0); break;
    }
    #undef Ps
    #undef Pu
    #undef Sh
}

static int do_common(struct sim_state *s, int32_t *Z, int32_t *rhs,
        int32_t *value, int loading, int storing, int arrow_right)
{
    int32_t * const r = arrow_right ? Z   : rhs;
    int32_t * const w = arrow_right ? rhs : Z  ;

    int rc = 0;
    if (loading) {
        rc = s->dispatch_op(s, OP_DATA_READ, *r, value);
    } else
        *value = *r;

    if (storing) {
        rc = s->dispatch_op(s, OP_WRITE, *w, value);
    } else if (w != &s->machine.regs[0])  // throw away write to reg 0
        *w = *value;

    return rc;
}

static int run_instruction(struct sim_state *s, const struct element *i, void *run_data)
{
    (void)run_data;
    int32_t * const ip = &s->machine.regs[15];
    int32_t rhs = 0;
    int32_t Y = 0, imm = 0, value = 0;
    enum op op = OP_BITWISE_OR;

    ++*ip;

    const struct instruction_type012 * const g = &i->insn.u.type012;
    const struct instruction_type3   * const v = &i->insn.u.type3;

    switch (i->insn.u.typeany.p) {
        case 0:
        case 1:
        case 2:
            Y   = s->machine.regs[g->y];
            imm = g->imm;
            op  = (enum op)g->op;
            break;
        case 3:
            Y   = 0;
            imm = v->imm;
            op  = OP_BITWISE_OR;
            break;
    }

    do_op(op, g->p, &rhs, s->machine.regs[g->x], Y, imm);
    return do_common(s, &s->machine.regs[g->z], &rhs, &value, g->dd == 3,
            g->dd == 1 || g->dd == 2, g->dd == 1);
}

static int updates_P(const struct insn_or_data i)
{
    struct instruction_typeany t = i.u.typeany;
    return (t.z == 15) && (t.dd == 0 || t.dd == 3);
}

int interp_step_sim(struct sim_state *s, const struct run_ops *ops,
        void **run_data, void *ops_data)
{
    if (ops->pre_fetch)
        if (ops->pre_fetch(s, ops_data))
            return 0;

    struct element i = { .insn = { .reladdr = 0 } };
    if (s->dispatch_op(s, OP_INSN_READ, s->machine.regs[15], &i.insn.u.word))
        return -1;

    i.insn.reladdr = s->machine.regs[15];

    if (ops->pre_insn)
        if (ops->pre_insn(s, &i, ops_data))
            return 0;

    if (run_instruction(s, &i, *run_data))
        return -1;

    if (ops->post_insn)
        if (ops->post_insn(s, &i, ops_data))
            return 0;

    // return  2 means "successful step, updated P register"
    // return  1 means "successful step, can continue as-is"
    // return  0 means "stopped because a hook told us to stop"
    // return -1 means "stopped because some error occurred"
    // TODO use an enumeration
    return updates_P(i.insn) ? 2 : 1;
}

int interp_run_sim(struct sim_state *s, const struct run_ops *ops,
        void **run_data, void *ops_data)
{
    int rc = 0;

    *run_data = NULL; // this runner needs no data yet
    do {
        rc = interp_step_sim(s, ops, run_data, ops_data);
    } while (rc > 0);

    return rc;
}

int load_sim(op_dispatcher *dispatch, void *sud, const struct format *f,
        void *ud, STREAM *in, int32_t load_address)
{
    struct element i;
    while (f->in(in, &i, ud) >= 0) {
        if (dispatch(sud, OP_WRITE, load_address + i.insn.reladdr, &i.insn.u.word))
            return -1;

        // Do a minimal word-at-a-time verification as we go.
        int32_t word = 0;
        if (dispatch(sud, OP_INSN_READ, load_address + i.insn.reladdr, &word))
            return -1;

        if (word != i.insn.u.word)
            return -1;
    }

    return 0;
}

/* vi: set ts=4 sw=4 et: */
