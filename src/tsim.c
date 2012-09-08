#include "plugin.h"

#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"
#include "ffi.h"

struct breakpoint {
    uint32_t addr;
    unsigned enabled:1;
};

#define KV_KEY_TYPE  uint32_t
#define KV_VAL_TYPE  struct breakpoint
#define KV_VAL_EMPTY NULL
#include "kv_int.h"

#include "debugger_global.h"
#include "debugger_parser.h"
#include "debugger_lexer.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <search.h>

#define RECIPES(_) \
    _(abort   , "call abort() when an illegal instruction is simulated") \
    _(prealloc, "preallocate memory (higher memory footprint, maybe faster)") \
    _(sparse  , "use sparse memory (lower memory footprint, maybe slower)") \
    _(serial  , "enable simple serial device and connect to stdio") \
    _(inittrap, "initialise unused memory to the illegal instruction") \
    _(nowrap  , "stop when PC wraps around 24-bit boundary") \
    _(plugin  , "load plugins specified through param mechanism") \
    //

#define DEFAULT_RECIPES(_) \
    _(sparse)   \
    _(serial)   \
    _(inittrap) \
    _(nowrap)   \
    _(plugin)   \
    //

#define Space(X) STR(X) " "

#define UsageDesc(Name,Desc) \
    "  " #Name ": " Desc "\n"

static int next_device(struct sim_state *s)
{
    while (s->machine.devices_count >= s->machine.devices_max) {
        s->machine.devices_max *= 2;
        s->machine.devices = realloc(s->machine.devices,
                s->machine.devices_max * sizeof *s->machine.devices);
    }

    return s->machine.devices_count++;
}

static int recipe_abort(struct sim_state *s)
{
    s->conf.abort = 1;
    return 0;
}

static int plugin_success(void *libhandle, const char *implstem, void *ud)
{
    int rc = 0;

    struct sim_state *s = ud;

    char buf[128];
    snprintf(buf, sizeof buf, "%s_add_device", implstem);
    void *ptr = dlsym(libhandle, buf);
    typedef int add_device(struct device **);
    add_device *adder = ALIASING_CAST(add_device,ptr);
    if (adder) {
        int index = next_device(s);
        s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
        rc |= adder(&s->machine.devices[index]);
    }

    return rc;
}

static int recipe_plugin(struct sim_state *s)
{
    return plugin_load("plugin", &s->conf.gops, &s->plugin_cookie, plugin_success, s);
}

static int recipe_prealloc(struct sim_state *s)
{
    int ram_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return ram_add_device(&s->machine.devices[index]);
}

static int recipe_sparse(struct sim_state *s)
{
    int sparseram_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return sparseram_add_device(&s->machine.devices[index]);
}

static int recipe_serial(struct sim_state *s)
{
    int serial_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return serial_add_device(&s->machine.devices[index]);
}

static int recipe_nowrap(struct sim_state *s)
{
    s->conf.nowrap = 1;
    return 0;
}

static int recipe_inittrap(struct sim_state *s)
{
    s->conf.should_init = 1;
    s->conf.initval = 0xffffffff;
    return 0;
}

static int find_device_by_addr(const void *_test, const void *_in)
{
    const uint32_t *addr = _test;
    const struct device * const *in = _in;

    if (!*in)
        return 0;

    if (*addr <= (*in)->bounds[1]) {
        if (*addr >= (*in)->bounds[0]) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return 1;
    }
}

static int dispatch_op(void *ud, int op, uint32_t addr, uint32_t *data)
{
    struct sim_state *s = ud;
    size_t count = s->machine.devices_count;
    struct device **device = bsearch(&addr, s->machine.devices, count, sizeof *device,
            find_device_by_addr);
    if (device == NULL || *device == NULL) {
        fprintf(stderr, "No device handles address %#x\n", addr);
        return -1;
    }
    // TODO don't send in the whole simulator state ? the op should have
    // access to some state, in order to redispatch and potentially use other
    // machine.devices, but it shouldn't see the whole state
    return (*device)->ops.op((*device)->cookie, op, addr, data);
}

static const char shortopts[] = "@:a:df:np:r:s:vhV";

static const struct option longopts[] = {
    { "options"    , required_argument, NULL, '@' },
    { "address"    , required_argument, NULL, 'a' },
    { "debug"      ,       no_argument, NULL, 'd' },
    { "format"     , required_argument, NULL, 'f' },
    { "scratch"    ,       no_argument, NULL, 'n' },
    { "param"      , required_argument, NULL, 'p' },
    { "recipe"     , required_argument, NULL, 'r' },
    { "start"      , required_argument, NULL, 's' },
    { "verbose"    ,       no_argument, NULL, 'v' },

    { "help"       ,       no_argument, NULL, 'h' },
    { "version"    ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

static const char *version()
{
    return "tsim version " STR(BUILD_NAME);
}

static int format_has_input(const struct format *f)
{
    return !!f->in;
}

static int usage(const char *me)
{
    char format_list[256];
    make_format_list(format_has_input, formats_count, formats, sizeof format_list, format_list, ", ");

    printf("Usage: %s [ OPTIONS ] image-file\n"
           "Options:\n"
           "  -@, --options=X       use options from file X\n"
           "  -a, --address=N       load instructions into memory at word address N\n"
           "  -d, --debug           start the simulator in debugger mode\n"
           "  -f, --format=F        select input format (%s)\n"
           "  -n, --scratch         don't run default recipes\n"
           "  -p, --param=X=Y       set parameter X to value Y\n"
           "  -r, --recipe=R        run recipe R (see list below)\n"
           "  -s, --start=N         start execution at word address N\n"
           "  -v, --verbose         increase verbosity of output\n"
           "\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           "\n"
           "Available recipes:\n"
           RECIPES(UsageDesc)
           "Default recipes:\n"
           "  " DEFAULT_RECIPES(Space)
           "\n"
           , me, format_list, version());

    return 0;
}

static int compare_devices_by_base(const void *_a, const void *_b)
{
    const struct device * const *a = _a;
    const struct device * const *b = _b;

    assert(("LHS of device comparison is not NULL", *a != NULL));
    assert(("RHS of device comparison is not NULL", *b != NULL));

    return (*a)->bounds[0] - (*b)->bounds[0];
}

static int devices_setup(struct sim_state *s)
{
    s->machine.devices_count = 0;
    s->machine.devices_max = 8;
    s->machine.devices = malloc(s->machine.devices_max * sizeof *s->machine.devices);

    return 0;
}

static int devices_finalise(struct sim_state *s)
{
    if (s->conf.verbose > 2) {
        assert(("device to be wrapped is not NULL", s->machine.devices[0] != NULL));
        int debugwrap_wrap_device(struct device **device);
        debugwrap_wrap_device(&s->machine.devices[0]);
        int debugwrap_unwrap_device(struct device **device);
        //debugwrap_unwrap_device(&s->machine.devices[0]);
    }

    // Devices must be in address order to allow later bsearch. Assume they do
    // not overlap (overlap is illegal).
    qsort(s->machine.devices, s->machine.devices_count,
            sizeof *s->machine.devices, compare_devices_by_base);

    for (unsigned i = 0; i < s->machine.devices_count; i++)
        if (s->machine.devices[i])
            s->machine.devices[i]->ops.init(&s->conf.gops, &s->plugin_cookie, &s->machine.devices[i]->cookie, 0);

    return 0;
}

static int devices_teardown(struct sim_state *s)
{
    for (unsigned i = 0; i < s->machine.devices_count; i++) {
        s->machine.devices[i]->ops.fini(s->machine.devices[i]->cookie);
        free(s->machine.devices[i]);
    }

    free(s->machine.devices);

    return 0;
}

static int devices_dispatch_cycle(struct sim_state *s)
{
    for (size_t i = 0; i < s->machine.devices_count; i++)
        if (s->machine.devices[i]->ops.cycle)
            if (s->machine.devices[i]->ops.cycle(s->machine.devices[i]->cookie))
                return 1;

    return 0;
}

static int run_recipe(struct sim_state *s, recipe r)
{
    return r(s);
}

static int run_recipes(struct sim_state *s)
{
    if (s->conf.run_defaults) {
        #define RUN_RECIPE(Recipe) run_recipe(s, recipe_##Recipe);
        DEFAULT_RECIPES(RUN_RECIPE);
    }

    list_foreach(recipe_book, b, s->recipes) {
        run_recipe(s, b->recipe);
        free(b);
    }

    return 0;
}

static int find_recipe_by_name(const void *_a, const void *_b)
{
    const struct format *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

static int add_recipe(struct sim_state *s, const char *name)
{
    static const struct recipe_entry {
        const char *name;
        recipe *recipe;
    } entries[] = {
        #define Entry(Name,Desc) { STR(Name), recipe_##Name },
        RECIPES(Entry)
    };
    size_t sz = countof(entries);

    struct recipe_entry *r = lfind(&(struct recipe_entry){ .name = name },
            entries, &sz, sizeof entries[0],
            find_recipe_by_name);

    if (r) {
        struct recipe_book *n = malloc(sizeof *n);
        n->next = s->recipes;
        n->recipe = r->recipe;
        s->recipes = n;

        return 0;
    } else {
        return 1;
    }
}

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
            if (s->dispatch_op(s, OP_READ, expr->val, &val))
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
            struct instruction i = { .u.word = val };
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
            struct instruction i;
            int32_t *ip = &dd->s->machine.regs[15];
            dd->s->dispatch_op(dd->s, OP_READ, *ip, &i.u.word);
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

static int run_debugger(struct sim_state *s, FILE *stream)
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

static int pre_insn(struct sim_state *s, struct instruction *i)
{
    if (s->conf.verbose > 0)
        printf("IP = 0x%06x\t", s->machine.regs[15]);

    if (s->conf.verbose > 1) {
        int len = print_disassembly(stdout, i, ASM_AS_INSN);
        fprintf(stdout, "%*s# ", 30 - len, "");
        print_disassembly(stdout, i, ASM_AS_DATA);
    }

    if (s->conf.verbose > 3) {
        fputs("\n", stdout);
        print_registers(stdout, s->machine.regs);
    }

    if (s->conf.verbose > 0)
        fputs("\n", stdout);

    return 0;
}

static int post_insn(struct sim_state *s, struct instruction *i)
{
    (void)i;
    return devices_dispatch_cycle(s);
}

static int find_format(const char *optarg, const struct format **f)
{
    size_t sz = formats_count;
    *f = lfind(&(struct format){ .name = optarg }, formats, &sz,
            sizeof formats[0], find_format_by_name);
    return !*f;
}

static int parse_args(struct sim_state *s, int argc, char *argv[]);

static int parse_opts_file(struct sim_state *s, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
        fatal(PRINT_ERRNO, "Options file '%s' not found", filename);

    char buf[1024], *p;
    while ((p = fgets(buf, sizeof buf, f))) {
        // trim newline
        int len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        int oi = optind;
        optind = 0;
        char *pbuf[] = { NULL, buf, NULL };
        parse_args(s, 2, pbuf);
        optind = oi;
    }

    int e = ferror(f);
    fclose(f);

    return e;
}

static int parse_args(struct sim_state *s, int argc, char *argv[])
{
    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'a': s->conf.load_addr = strtol(optarg, NULL, 0); break;
            case 'd': s->conf.debugging = 1; break;
            case 'f': if (find_format(optarg, &s->conf.fmt)) exit(usage(argv[0])); break;
            case '@': if (parse_opts_file(s, optarg)) fatal(PRINT_ERRNO, "Error in opts file"); break;
            case 'n': s->conf.run_defaults = 0; break;
            case 'p': param_add(&s->conf.params, optarg); break;
            case 'r': add_recipe(s, optarg); break;
            case 's': s->conf.start_addr = strtol(optarg, NULL, 0); break;
            case 'v': s->conf.verbose++; break;

            case 'V': puts(version()); exit(EXIT_SUCCESS);
            case 'h': usage(argv[0]) ; exit(EXIT_SUCCESS);
            default : usage(argv[0]) ; exit(EXIT_FAILURE);
        }
    }

    return 0;
}

static int plugin_param_get(void *cookie, char *key, const char **val)
{
    struct plugin_cookie *c = cookie;
    return param_get(c->param, key, val);
}

static int plugin_param_set(void *cookie, char *key, char *val, int free_value)
{
    struct plugin_cookie *c = cookie;
    return param_set(c->param, key, val, free_value);
}

int main(int argc, char *argv[])
{
    int rc = EXIT_SUCCESS;

    struct sim_state _s = {
        .conf = {
            .verbose      = 0,
            .run_defaults = 1,
            .debugging    = 0,
            .start_addr   = RAM_BASE,
            .load_addr    = RAM_BASE,
            .fmt          = &formats[0],
            .params = {
                .params_size  = DEFAULT_PARAMS_COUNT,
                .params_count = 0,
                .params       = calloc(DEFAULT_PARAMS_COUNT, sizeof *_s.conf.params.params),
            },
            .gops         = {
                .fatal = fatal_,
                .debug = debug_,
                .param_get = plugin_param_get,
                .param_set = plugin_param_set,
            },
        },
        .dispatch_op = dispatch_op,
        .plugin_cookie = {
            .param = &_s.conf.params,
        },
    }, *s = &_s;

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        return EXIT_FAILURE;
    }

    parse_args(s, argc, argv);

    if (optind >= argc) {
        fatal(DISPLAY_USAGE, "No input files specified on the command line");
    } else if (argc - optind > 1) {
        fatal(DISPLAY_USAGE, "More than one input file specified on the command line");
    }

    FILE *in = stdin;

    if (!strcmp(argv[optind], "-")) {
        in = stdin;
    } else {
        in = fopen(argv[optind], "rb");
        if (!in) {
            char buf[128];
            snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[optind]);
            fatal(PRINT_ERRNO, buf);
        }
    }

    devices_setup(s);
    run_recipes(s);
    devices_finalise(s);

    load_sim(s->dispatch_op, s, s->conf.fmt, in, s->conf.load_addr);
    s->machine.regs[15] = s->conf.start_addr & PTR_MASK;

    struct run_ops ops = {
        .pre_insn = pre_insn,
        .post_insn = post_insn,
    };

    if (s->conf.debugging)
        run_debugger(s, stdin);
    else
        run_sim(s, &ops);

    if (in)
        fclose(in);

    devices_teardown(s);

    param_destroy(&s->conf.params);

    return rc;
}

