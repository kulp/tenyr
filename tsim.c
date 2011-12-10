#include <stdlib.h>
#include <stdio.h>

#include "ops.h"

#define PTR_MASK ~(-1 << 24)

struct state {
    uint32_t regs[16];
    uint32_t mem[1 << 24];
};

int run_instruction(struct state *s, struct instruction *i)
{
    int sextend = 0;
    uint32_t _ip, *ip = &s->regs[15];

    switch (i->u._xxxx.t) {
        case 0b1011:
        case 0b1001:
            sextend = 1;
        case 0b1010:
        case 0b1000: {
            struct instruction_load_immediate_signed *g = &i->u._10x1;
            uint32_t *Z = &s->regs[g->z];
            if (g->d) Z = &s->mem[*Z & PTR_MASK];
            uint32_t imm = sextend ? g->imm : (int32_t)(uint32_t)g->imm;
            *Z = imm;
            break;
        }
        case 0b0000 ... 0b0111: {
            struct instruction_general *g = &i->u._0xxx;
            int ld = g->dd & 2;
            int rd = g->dd & 1;
            uint32_t *Z = &s->regs[g->z];
            uint32_t  X =  s->regs[g->x];
            uint32_t  Y =  s->regs[g->y];
            int32_t   I = g->imm;
            uint32_t _rhs, *rhs = &_rhs;

            switch (g->op) {
                case OP_BITWISE_OR          : *rhs =  (X  |  Y) + I; break;
                case OP_BITWISE_AND         : *rhs =  (X  &  Y) + I; break;
                case OP_ADD                 : *rhs =  (X  +  Y) + I; break;
                case OP_MULTIPLY            : *rhs =  (X  *  Y) + I; break;
                case OP_MODULUS             : *rhs =  (X  %  Y) + I; break;
                case OP_SHIFT_LEFT          : *rhs =  (X  << Y) + I; break;
                case OP_COMPARE_LTE         : *rhs =  (X  <= Y) + I; break;
                case OP_COMPARE_EQ          : *rhs =  (X  == Y) + I; break;
                case OP_BITWISE_NOR         : *rhs = ~(X  |  Y) + I; break;
                case OP_BITWISE_NAND        : *rhs = ~(X  &  Y) + I; break;
                case OP_BITWISE_XOR         : *rhs =  (X  ^  Y) + I; break;
                case OP_ADD_NEGATIVE_Y      : *rhs =  (X  + -Y) + I; break;
                case OP_XOR_INVERT_X        : *rhs =  (X  ^ ~Y) + I; break;
                case OP_SHIFT_RIGHT_LOGICAL : *rhs =  (X  >> Y) + I; break;
                case OP_COMPARE_GT          : *rhs =  (X  >  Y) + I; break;
                case OP_COMPARE_NE          : *rhs =  (X  != Y) + I; break;
            }

            if (ld) Z   = &s->mem[*Z   & PTR_MASK];
            if (rd) rhs = &s->mem[*rhs & PTR_MASK];

            {
                uint32_t *r, *w;
                if (g->r)
                    r = Z, w = rhs;
                else
                    w = Z, r = rhs;

                if (w == ip) ip = &_ip; // throw away later update
                *w = *r;
            }


            break;
        }
        default: abort();
    }

    ++*ip;

    return 0;
}

int main(int argc, char *argv[])
{
    struct state *s = calloc(1, sizeof *s);

    run_instruction(s, &(struct instruction){ 0x12345678 });
    run_instruction(s, &(struct instruction){ 0x80f23456 });
    run_instruction(s, &(struct instruction){ 0x90f23456 });
    run_instruction(s, &(struct instruction){ 0xa0f23456 });
    run_instruction(s, &(struct instruction){ 0xb0f23456 });
    run_instruction(s, &(struct instruction){ 0x7fedc000 });
    run_instruction(s, &(struct instruction){ 0xffffffff }); // illegal instruction

    return 0;
}

