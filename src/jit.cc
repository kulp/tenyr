#include "ops.h"
#include "common.h"
#include "sim.h"
#include "asmjit/asmjit.h"
#include <cassert>
#include <search.h>

using namespace asmjit;

typedef void Block(void *cookie, int32_t *registers);

struct jit_state {
    void *nested_run_data;
    void *sim_state;
    JitRuntime *runtime;
    int run_count_threshold;
    X86Compiler *cc;
};

struct ops_state {
    struct jit_state *js;
    struct run_ops ops;
    void *nested_ops_data;
    void *basic_blocks;
    struct basic_block *curr_bb;
};

struct basic_block {
    int run_count;
    unsigned complete;
    int32_t base;
    uint32_t len;
    Block *compiled;
    int32_t *cache;
};

extern "C" void jit_init(struct jit_state **state)
{
    struct jit_state *s = *state = new jit_state;
    s->runtime = new JitRuntime;
    s->run_count_threshold = 10; // arbitrary, reconfigurable
    s->cc = new X86Compiler(s->runtime);
}

extern "C" void jit_fini(struct jit_state *state)
{
    delete state->cc;
    delete state->runtime;
    delete state;
}

// XXX permit fetch to express failure
static int32_t fetch(void *cookie, int32_t addr)
{
    struct sim_state *s = (struct sim_state*)cookie;
    int32_t data;
    s->dispatch_op(s, OP_DATA_READ, addr, (uint32_t*)&data);
    return data;
}

static void store(void *cookie, int32_t addr, int32_t value)
{
    struct sim_state *s = (struct sim_state*)cookie;
    s->dispatch_op(s, OP_WRITE, addr, (uint32_t*)&value);
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
    switch (op) {
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
            switch (op) {
                case OP_COMPARE_LT: cc.cmovl (A, T); break;
                case OP_COMPARE_EQ: cc.cmove (A, T); break;
                case OP_COMPARE_GE: cc.cmovge(A, T); break;
            }
            break;

        default:
            assert(!"Bad op");
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
    X86Compiler &c = *js->cc;

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

static int bb_by_base(const void *a_, const void *b_)
{
    struct basic_block *a = (struct basic_block*)a_,
                       *b = (struct basic_block*)b_;

    return a->base - b->base;
}

static int pre_insn_hook(struct sim_state *s, const struct element *i, void *ud)
{
    struct ops_state *o = (struct ops_state*)ud;
    if (!o->curr_bb) {
        struct basic_block nb = { 0, 0, i->insn.reladdr, 0, NULL, NULL };
        struct basic_block **f = (struct basic_block**)tsearch(&nb, &o->basic_blocks, bb_by_base);
        if (*f == &nb) {
            *f = (struct basic_block *)malloc(sizeof **f);
            **f = nb;
        }
        o->curr_bb = *f;
    }

    struct basic_block *bb = o->curr_bb;
    if (bb->compiled)
        return 1; // indicate to jit_run_sim that JIT should be used

    if (bb->run_count == o->js->run_count_threshold) {
        // Cache the instructions we receive so they are ready for compilation
        if (!bb->cache)
            bb->cache = new int32_t[bb->len];
        bb->cache[(uint32_t)i->insn.reladdr - (uint32_t)bb->base] = i->insn.u.word;
    } else if (bb->run_count > o->js->run_count_threshold) {
        assert(bb->complete);
        bb->compiled = jit_gen_block(o->js, bb->len, bb->cache);
        delete [] bb->cache;
        return 1; // indicate to jit_run_sim that JIT should be used
    }

    return o->ops.pre_insn(s, i, o->nested_ops_data);
}

static int post_insn_hook(struct sim_state *s, const struct element *i, void *ud)
{
    struct ops_state *o = (struct ops_state*)ud;

    int dd = i->insn.u.typeany.dd;
    if ((dd == 0 || dd == 3) && i->insn.u.typeany.z == 0xf) { // P is being updated
        struct basic_block *bb = o->curr_bb;
        bb->len = (uint32_t)i->insn.reladdr - (uint32_t)bb->base + 1;
        bb->complete = 1;
        bb->run_count++;
        o->curr_bb = NULL;
        o->ops.post_insn(s, i, o->nested_ops_data); // allow hook to run one last time
        return 1;
    }

    return o->ops.post_insn(s, i, o->nested_ops_data);
}

extern "C" int jit_run_sim(struct sim_state *s, struct run_ops *ops, void **run_data, void *ops_data)
{
    struct jit_state *js;
    jit_init(&js);
    *run_data = js;
    js->sim_state = s;

    const void *val = NULL;
    if (param_get(s->conf.params, "tsim.jit.run_count_threshold", 1, &val)) {
        char *next = NULL;
        if (val) {
            int v = strtol((char*)val, &next, 0);
            if (next != val && next != NULL && *next == '\0' && v > 1)
                js->run_count_threshold = v;
        }
    }

    struct ops_state ops_state = { 0 }, *o = &ops_state;
    o->js = js;
    o->ops.pre_insn = ops->pre_insn;
    o->ops.post_insn = ops->post_insn;
    o->nested_ops_data = ops_data;

    struct run_ops wrappers = { 0 };
    wrappers.pre_insn = pre_insn_hook;
    wrappers.post_insn = post_insn_hook;

    typedef int dispatch_cycle(struct sim_state *s);
    dispatch_cycle *pump = (dispatch_cycle*)dlsym(RTLD_DEFAULT, "devices_dispatch_cycle");
    sim_runner *interp = (sim_runner*)dlsym(RTLD_DEFAULT, "interp_run_sim");
    if (!interp || !pump)
        fatal(PRINT_ERRNO, "Failed to find interpreter");

    int rc = 0;
    do {
        if (o->curr_bb && o->curr_bb->compiled) {
            o->curr_bb->compiled(s, s->machine.regs);
            s->insns_executed += o->curr_bb->len;
            o->curr_bb->run_count++;
            o->curr_bb = NULL;
        }
        rc = interp(s, &wrappers, &js->nested_run_data, o);
        pump(s); // no longer a "cycle" but periodic
    } while (rc >= 0);

    jit_fini(js);
    return 0;
}

