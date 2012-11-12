#include "debugger_global.h"
#include "debugger_parser.h"
#include "debugger_lexer.h"

#include "sim.h"
#include "ffi.h"

#include <assert.h>
#include <stdint.h>

struct breakpoint {
    uint32_t addr;
    unsigned enabled:1;
};

#define KV_KEY_TYPE  uint32_t
#define KV_VAL_TYPE  struct breakpoint
#define KV_VAL_EMPTY NULL
#include "kv_int.h"

static int set_breakpoint(void **breakpoints, int32_t addr)
{
    struct breakpoint *old = kv_int_get(breakpoints, addr);
    if (old) {
        if (!old->enabled) {
            printf("Enabled previous breakpoint at %#lx\n", (long unsigned)old->addr);
            old->enabled = 1;
        } else {
            printf("Breakpoint already exists at %#lx\n", (long unsigned)old->addr);
        }
    } else {
        struct breakpoint bp = {
            .enabled = 1,
            .addr    = addr,
        };
        kv_int_put(breakpoints, addr, &bp);
        printf("Added breakpoint at %#lx\n", (long unsigned)addr);
    }

    return 0;
}

static int delete_breakpoint(void **breakpoints, int32_t addr)
{
    struct breakpoint *old = kv_int_remove(breakpoints, addr);
    if (old) {
        printf("Removed breakpoint at %#lx\n", (long unsigned)addr);
    } else {
        printf("No breakpoint at %#lx\n", (long unsigned)addr);
    }

    return 0;
}

static int matches_breakpoint(struct machine_state *m, void *cud)
{
    uint32_t pc = m->regs[15];
    void *breakpoints = cud;
    struct breakpoint *c = kv_int_get(&breakpoints, pc);
    return c && c->enabled && c->addr == pc;
}

static int print_expr(struct sim_state *s, struct debug_expr *expr, int fmt)
{
    static const char *fmts[DISP_max] ={
        [DISP_NULL] = "%d",     // this is used by default
        [DISP_DEC ] = "%d",
        [DISP_HEX ] = "0x%08x",
    };

    uint32_t val = 0x0badcafe;

    switch (expr->type) {
        case EXPR_MEM: {
            if (s->dispatch_op(s, OP_DATA_READ, expr->val, &val))
                return -1;
            break;
        }
        case EXPR_REG:
            assert(("Register name in range",
                    expr->val >= 0 && expr->val < 16));
            val = s->machine.regs[expr->val];
            break;
        default:
            fatal(0, "Invalid print type code %d\n", expr->type);
    }

    switch (fmt) {
        case DISP_INST: {
            struct element i = { .insn.u.word = val };
            print_disassembly(stdout, &i, 0);
            fputc('\n', stdout);
            return 0;
        }
        case DISP_NULL:
        case DISP_HEX:
        case DISP_DEC:
            printf(fmts[fmt], val);
            fputc('\n', stdout);
            return 0;
        default:
            fatal(0, "Invalid format code %d", fmt);
    }

    return -1;
}

static int get_info(struct sim_state *s, struct debug_cmd *c)
{
    if (!strncmp(c->arg.str, "registers", sizeof c->arg.str)) {
        print_registers(stdout, s->machine.regs);
        return 0;
    } else {
        fprintf(stderr, "Invalid argument `%s' to info", c->arg.str);
        return -1;
    }

    return -1;
}

static int add_display(struct debugger_data *dd, struct debug_expr expr, int fmt)
{
    struct debug_display *disp = calloc(1, sizeof *disp);
    disp->expr = expr;
    disp->fmt = fmt;
    disp->next = dd->displays;
    dd->displays = disp;
    dd->displays_count++;

    return 0;
}

static int show_displays(struct debugger_data *dd)
{
    int i = dd->displays_count;
    list_foreach(debug_display,disp,dd->displays) {
        printf("display %d : ", i--);
        print_expr(dd->s, &disp->expr, disp->fmt);
    }

    return 0;
}

static int debugger_step(struct debugger_data *dd)
{
    int done = 0;

    struct debug_cmd *c = &dd->cmd;
    tdbg_prompt(dd, stdout);
    tdbg_parse(dd); // TODO handle errors

    switch (c->code) {
        case CMD_NULL:
            break;
        case CMD_GET_INFO:
            get_info(dd->s, c);
            break;
        case CMD_DELETE_BREAKPOINT:
            delete_breakpoint(&dd->breakpoints, c->arg.expr.val);
            break;
        case CMD_SET_BREAKPOINT:
            set_breakpoint(&dd->breakpoints, c->arg.expr.val);
            break;
        case CMD_DISPLAY:
            add_display(dd, c->arg.expr, c->arg.fmt);
            break;
        case CMD_PRINT:
            print_expr(dd->s, &c->arg.expr, c->arg.fmt);
            break;
        case CMD_CONTINUE: {
            int32_t *ip = &dd->s->machine.regs[15];
            printf("Continuing @ %#x ... ", *ip);
            tf_run_until(dd->s, *ip, TF_IGNORE_FIRST_PREDICATE,
                    matches_breakpoint, dd->breakpoints);
            printf("stopped @ %#x\n", *ip);
            show_displays(dd);
            break;
        }
        case CMD_STEP_INSTRUCTION: {
            struct element i;
            int32_t *ip = &dd->s->machine.regs[15];
            dd->s->dispatch_op(dd->s, OP_INSN_READ, *ip, &i.insn.u.word);
            printf("Stepping @ %#x ... ", *ip);
            if (run_instruction(dd->s, &i)) {
                printf("failed (P = %#x)\n", *ip);
                return 1;
            }
            printf("stopped @ %#x\n", *ip);
            show_displays(dd);
            break;
        }
        case CMD_QUIT:
            done = 1;
            break;
        default:
            fatal(0, "Invalid debugger command code %d", c->code);
    }

    return done;
}

int run_debugger(struct sim_state *s, FILE *stream)
{
    struct debugger_data _dd = { .s = s }, *dd = &_dd;
    kv_int_init(&dd->breakpoints);

    tdbg_lex_init(&dd->scanner);
    tdbg_set_extra(dd, dd->scanner);
    tdbg_set_in(stream, dd->scanner);

    int done = 0;
    while (!done && !feof(stream))
        done = debugger_step(dd);

    list_foreach(debug_display,disp,dd->displays)
        free(disp);

    tdbg_lex_destroy(dd->scanner);

    return 0;
}

