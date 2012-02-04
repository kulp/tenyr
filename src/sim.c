#include "sim.h"
#include "common.h"

#include <assert.h>
#include <stdlib.h>

int run_instruction(struct state *s, struct instruction *i)
{
    int32_t *ip = &s->machine.regs[15];

    assert(("PC within address space", !(*ip & ~PTR_MASK)));

    int32_t *Z;
    int32_t _rhs, *rhs = &_rhs;
    uint32_t value;
    int deref_lhs, deref_rhs;
    int reversed;

    switch (i->u._xxxx.t) {
        case 0x0:
        case 0x1:
        case 0x2:
        case 0x3:
        case 0x4:
        case 0x5:
        case 0x6:
        case 0x7: {
            struct instruction_general *g = &i->u._0xxx;
            Z = &s->machine.regs[g->z];
            int32_t  Xs = s->machine.regs[g->x];
            uint32_t Xu = s->machine.regs[g->x];
            int32_t  Ys = s->machine.regs[g->y];
            uint32_t Yu = s->machine.regs[g->y];
            int32_t  Is = SEXTEND(12, g->imm);

            // note Is, not Iu, since immediate is always sign-extended
            int32_t  Os = (g->p == 0) ? Ys : Is;
            uint32_t Ou = (g->p == 0) ? Yu : (uint32_t)Is;
            int32_t  As = (g->p == 0) ? Is : Ys;

            deref_rhs = g->dd & 1;
            deref_lhs = g->dd == 2;
            reversed  = g->dd == 3;

            switch (g->op) {
                case OP_ADD                 : *rhs =  (Xs  +  Os) + As; break;
                case OP_ADD_NEGATIVE_Y      : *rhs =  (Xs  + -Os) + As; break;
                case OP_MULTIPLY            : *rhs =  (Xs  *  Os) + As; break;

                case OP_SHIFT_LEFT          : *rhs =  (Xu  << Ou) + As; break;
                case OP_SHIFT_RIGHT_LOGICAL : *rhs =  (Xu  >> Ou) + As; break;

                case OP_COMPARE_EQ          : *rhs = -(Xs  == Os) + As; break;
                case OP_COMPARE_NE          : *rhs = -(Xs  != Os) + As; break;
                case OP_COMPARE_LTE         : *rhs = -(Xs  <= Os) + As; break;
                case OP_COMPARE_GT          : *rhs = -(Xs  >  Os) + As; break;

                case OP_BITWISE_AND         : *rhs =  (Xu  &  Ou) + As; break;
                case OP_BITWISE_NAND        : *rhs = ~(Xu  &  Ou) + As; break;
                case OP_BITWISE_OR          : *rhs =  (Xu  |  Ou) + As; break;
                case OP_BITWISE_NOR         : *rhs = ~(Xu  |  Ou) + As; break;
                case OP_BITWISE_XOR         : *rhs =  (Xu  ^  Ou) + As; break;
                case OP_XOR_INVERT_X        : *rhs =  (Xu  ^ ~Ou) + As; break;

                case OP_RESERVED:
                    fatal(0, "Encountered reserved opcode");
            }

            break;
        }
        default:
            if (s->conf.abort)
                abort();
            else
                return 1;
    }

    // common activity block
    {
        uint32_t r_addr = (reversed ? *Z   : *rhs) & PTR_MASK;
        uint32_t w_addr = (reversed ? *rhs : *Z  ) & PTR_MASK;

        int read_mem  = reversed ? deref_lhs : deref_rhs;
        int write_mem = reversed ? deref_rhs : deref_lhs;

        int32_t *r = reversed ? Z   : rhs;
        int32_t *w = reversed ? rhs : Z;

        if (read_mem)
            s->dispatch_op(s, OP_READ, r_addr, &value);
        else
            value = *r;

        if (write_mem)
            s->dispatch_op(s, OP_WRITE, w_addr, &value);
        else if (w != &s->machine.regs[0])  // throw away write to reg 0
            *w = value;

        if (w != ip) {
            ++*ip;
            if (*ip & ~PTR_MASK && s->conf.nowrap) {
                if (s->conf.abort) {
                    abort();
                } else {
                    return 1;
                }
            }

            *ip &= PTR_MASK;
        }
    }

    return 0;
}


