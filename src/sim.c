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
            deref_rhs = g->dd & 1;
            deref_lhs = g->dd & 2;
            reversed = !!g->r;

            switch (g->op) {
                case OP_ADD                 : *rhs =  (Xs  +  Ys) + Is; break;
                case OP_ADD_NEGATIVE_Y      : *rhs =  (Xs  + -Ys) + Is; break;
                case OP_MULTIPLY            : *rhs =  (Xs  *  Ys) + Is; break;

                case OP_SHIFT_LEFT          : *rhs =  (Xu  << Yu) + Is; break;
                case OP_SHIFT_RIGHT_LOGICAL : *rhs =  (Xu  >> Yu) + Is; break;

                case OP_COMPARE_EQ          : *rhs = -(Xs  == Ys) + Is; break;
                case OP_COMPARE_NE          : *rhs = -(Xs  != Ys) + Is; break;
                case OP_COMPARE_LTE         : *rhs = -(Xs  <= Ys) + Is; break;
                case OP_COMPARE_GT          : *rhs = -(Xs  >  Ys) + Is; break;

                case OP_BITWISE_AND         : *rhs =  (Xu  &  Yu) + Is; break;
                case OP_BITWISE_NAND        : *rhs = ~(Xu  &  Yu) + Is; break;
                case OP_BITWISE_OR          : *rhs =  (Xu  |  Yu) + Is; break;
                case OP_BITWISE_NOR         : *rhs = ~(Xu  |  Yu) + Is; break;
                case OP_BITWISE_XOR         : *rhs =  (Xu  ^  Yu) + Is; break;
                case OP_XOR_INVERT_X        : *rhs =  (Xu  ^ ~Yu) + Is; break;

                case OP_RESERVED:
                    fatal(0, "Encountered reserved opcode");
            }

            break;
        }
        case 0x8:
        case 0x9:
        case 0xa:
        case 0xb: {
            struct instruction_load_immediate *g = &i->u._10xx;
            Z = &s->machine.regs[g->z];
            int32_t _imm = g->imm;
            deref_rhs = g->dd & 1;
            deref_lhs = g->dd & 2;
            reversed = 0;
            rhs = &_imm;

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


