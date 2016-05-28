#include "ops.h"
#include "common.h"
#include "asmjit/asmjit.h"

#include "jit.h"

using namespace asmjit;

extern "C" void jit_init(struct jit_state **state)
{
    struct jit_state *s = *state = new jit_state;
    s->runtime = new JitRuntime;
    s->run_count_threshold = 10; // arbitrary, reconfigurable
    s->cc = new X86Compiler((JitRuntime*)s->runtime);
}

extern "C" void jit_fini(struct jit_state *state)
{
    delete (X86Compiler*)state->cc;
    delete (JitRuntime*)state->runtime;
    delete state;
}

static const FuncPrototype store_proto = FuncBuilder3<void   , void *, int32_t, int32_t>();
static const FuncPrototype fetch_proto = FuncBuilder2<int32_t, void *, int32_t         >();

static inline void create_store(X86Compiler &cc, X86GpVar &cookie, X86GpVar &addr, X86GpVar &data)
{
    X86CallNode *call = cc.call(imm_ptr((void*)store), kFuncConvHost, store_proto);
    call->setArg(0, cookie);
    call->setArg(1, addr);
    call->setArg(2, data);
}

static inline void create_fetch(X86Compiler &cc, X86GpVar &cookie, X86GpVar &addr, X86GpVar &data)
{
    X86CallNode *call = cc.call(imm_ptr((void*)fetch), kFuncConvHost, fetch_proto);
    call->setArg(0, cookie);
    call->setArg(1, addr);
    call->setRet(0, data);
}

static void buildOp(X86Compiler &cc, int op, X86GpVar &T, X86GpVar &A, X86GpVar &B, X86GpVar &C)
{
    switch (op & 0xf) {
        case OP_ADD             : cc.add (A, B);    break;
        case OP_SUBTRACT        : cc.sub (A, B);    break;
        case OP_BITWISE_ANDN    : cc.not_(B);       /* FALLTHROUGH */
        case OP_BITWISE_AND     : cc.and_(A, B);    break;
        case OP_BITWISE_ORN     : cc.not_(B);       /* FALLTHROUGH */
        case OP_BITWISE_OR      : cc.or_ (A, B);    break;
        case OP_BITWISE_XOR     : cc.xor_(A, B);    break;
        case OP_MULTIPLY        : cc.imul(A, B);    break;

        case OP_SHIFT_RIGHT_LOGIC:
            cc.shr   (A, B);
            cc.and_  (B, Imm(~31));
            cc.mov   (T, Imm(0));
            cc.cmovnz(A, T);            // zeros if shift-amount is larger than 31
            break;
        case OP_SHIFT_RIGHT_ARITH:
            cc.mov   (T, B);
            cc.and_  (T, Imm(~31));
            cc.mov   (T, 31);
            cc.cmovnz(B, T);            // convert to a shift by 31 if > 31
            cc.sar   (A, B);
            break;
        case OP_SHIFT_LEFT:
            cc.sal   (A, B);
            cc.and_  (B, Imm(~31));
            cc.mov   (T, Imm(0));
            cc.cmovnz(A, T);            // zeros if shift-amount is larger than 31
            break;

        case OP_PACK:
            cc.shl (A, Imm(12));
            cc.and_(B, Imm(0xfff));
            cc.or_ (A, B);
            break;

        case OP_TEST_BIT:
            cc.bt (A, B);
            cc.mov(A, Imm(0));
            cc.sbb(A, A);
            break;

        case OP_COMPARE_LT: /* FALLTHROUGH */
        case OP_COMPARE_EQ: /* FALLTHROUGH */
        case OP_COMPARE_GE:
            cc.mov(T, Imm(-1));
            cc.cmp(A, B);
            cc.mov(A, Imm(0)); // don't use xor, it clears the flags we need
            if (op == OP_COMPARE_LT) cc.cmovl (A, T);
            if (op == OP_COMPARE_EQ) cc.cmove (A, T);
            if (op == OP_COMPARE_GE) cc.cmovge(A, T);
            break;
    }
}

static int buildInstruction(X86Compiler &cc, X86GpVar &ck, X86GpVar &regs,
        const int32_t &instruction, int32_t offset)
{
    X86GpVar *a = NULL, *b = NULL, *c = NULL;
    X86GpVar tmp(cc, kVarTypeInt32, "tmp"),
               x(cc, kVarTypeInt32, "x"),
               y(cc, kVarTypeInt32, "y"),
               i(cc, kVarTypeInt32, "i");

    union insn_or_data::insn &u = *(union insn_or_data::insn *)&instruction;
    struct insn_or_data::insn::instruction_type012 &t = u.type012;
    struct insn_or_data::insn::instruction_type3   &v = u.type3;
    int op = t.op;

#define Reg(N) (X86Mem(regs, 4*(N), 0))

    if (t.x) {
        cc.mov(x, Reg(t.x));
        if (t.x == 15)
            cc.add(x, Imm(offset + 1));
    } else {
        cc.xor_(x, x);
    }

    if (t.p == 3) {
        a = &x;
        cc.add(*a, Imm(SEXTEND32(MEDIUM_IMMEDIATE_BITWIDTH,v.imm)));
    } else {
        if (t.y) {
            cc.mov(y, Reg(t.y));
            if (t.y == 15)
                cc.add(y, Imm(offset + 1));
        } else {
            cc.xor_(y, y);
        }
        cc.mov(i, Imm(SEXTEND32(SMALL_IMMEDIATE_BITWIDTH,t.imm)));

        switch (t.p) {
            case 0: a = &x; b = &y; c = &i; break;
            case 1: a = &x; b = &i; c = &y; break;
            case 2: a = &i; b = &x; c = &y; break;
        }

        buildOp(cc, op, tmp, *a, *b, *c);
        cc.add(*a, *c);
    }

    // TODO don't store registers until we need to
    // TODO don't do operations that are identity operations with *a
    // TODO don't load registers unless necessary

    switch (t.dd) {
        case 0:                                if (t.z) cc.mov(Reg(t.z),  *a); break;
        case 1: cc.mov(tmp, Reg(t.z));         create_store(cc, ck,  *a, tmp); break;
        case 2: cc.mov(tmp, Reg(t.z));         create_store(cc, ck,  tmp, *a); break;
        case 3: create_fetch(cc, ck, *a, tmp); if (t.z) cc.mov(Reg(t.z), tmp); break;
    }

    return t.z == 15;
}

extern "C" Block *jit_gen_block(void *cookie, int len, int32_t *instructions)
{
    struct jit_state *js = (struct jit_state*)cookie;
    X86Compiler &c = *(X86Compiler*)js->cc;

    c.addFunc(kFuncConvHost, FuncBuilder2<void, void*, int32_t*>());

    X86GpVar ck  (c, kVarTypeUIntPtr, "cookie");
    X86GpVar regs(c, kVarTypeUIntPtr, "regs");
    c.setArg(0, ck);
    c.setArg(1, regs);

    for (int i = 0; i < len; i++)
        buildInstruction(c, ck, regs, instructions[i], i);

    c.endFunc();

    return (Block*)c.make();
}
