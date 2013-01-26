#include "plugin.h"

#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <search.h>

#define RECIPES(_) \
    _(abort   , "abort() on illegal instruction or memory address") \
    _(prealloc, "preallocate memory (higher memory footprint, maybe faster)") \
    _(sparse  , "use sparse memory (lower memory footprint, maybe slower)") \
    _(serial  , "enable simple serial device and connect to stdio") \
    _(nowrap  , "stop when PC wraps around 24-bit boundary") \
    _(plugin  , "load plugins specified through param mechanism") \
    //

#define DEFAULT_RECIPES(_) \
    _(sparse)   \
    _(serial)   \
    _(nowrap)   \
    _(plugin)   \
    //

#define Space(X) STR(X) " "

#define UsageDesc(Name,Desc) \
    "  " #Name ": " Desc "\n"

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
    make_format_list(format_has_input, tenyr_asm_formats_count,
            tenyr_asm_formats, sizeof format_list, format_list, ", ");

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
           "  -V, --version         print the string `%s'\n"
           "\n"
           "Available recipes:\n"
           RECIPES(UsageDesc)
           "Default recipes:\n"
           "  " DEFAULT_RECIPES(Space)
           "\n"
           , me, format_list, version());

    return 0;
}

static int recipe_abort(struct sim_state *s)
{
    s->conf.abort = 1;
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

static int plugin_success(void *libhandle, int inst, const char *parent, const
    char *implstem, void *ud)
{
    int rc = 0;
    (void)inst;
    (void)parent;

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
    return plugin_load("plugin", &s->plugin_cookie, plugin_success, s);
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

struct recipe_entry {
    const char *name;
    recipe *recipe;
};

static int find_recipe_by_name(const void *_a, const void *_b)
{
    const struct recipe_entry *a = _a, *b = _b;
    return strcmp(a->name, b->name);
}

static int add_recipe(struct sim_state *s, const char *name)
{
    static const struct recipe_entry entries[] = {
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

static int plugin_param_get(const struct plugin_cookie *cookie, char *key, const char **val)
{
    return param_get(cookie->param, key, val);
}

static int plugin_param_set(struct plugin_cookie *cookie, char *key, char *val, int free_value)
{
    return param_set(cookie->param, key, val, free_value);
}

static int find_format(const char *optarg, const struct format **f)
{
    size_t sz = tenyr_asm_formats_count;
    *f = lfind(&(struct format){ .name = optarg }, tenyr_asm_formats, &sz,
            sizeof tenyr_asm_formats[0], find_format_by_name);
    return !*f;
}

static int parse_args(struct sim_state *s, int argc, char *argv[]);

static int parse_opts_file(struct sim_state *s, const char *filename)
{
    FILE *f = fopen(filename, "r");
    if (!f)
        fatal(PRINT_ERRNO, "Options file `%s' not found", filename);

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
            case 'r': if (add_recipe(s, optarg)) exit(usage(argv[0])); break;
            case 's': s->conf.start_addr = strtol(optarg, NULL, 0); break;
            case 'v': s->conf.verbose++; break;

            case 'V': puts(version()); exit(EXIT_SUCCESS);
            case 'h': usage(argv[0]) ; exit(EXIT_SUCCESS);
            default : usage(argv[0]) ; exit(EXIT_FAILURE);
        }
    }

    return 0;
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
            .fmt          = &tenyr_asm_formats[0],
            .params = {
                .params_size  = DEFAULT_PARAMS_COUNT,
                .params_count = 0,
                .params       = calloc(DEFAULT_PARAMS_COUNT, sizeof *_s.conf.params.params),
            },
        },
        .dispatch_op = dispatch_op,
        .plugin_cookie = {
            .param = &_s.conf.params,
            .gops         = {
                .fatal = fatal_,
                .debug = debug_,
                .param_get = plugin_param_get,
                .param_set = plugin_param_set,
            },
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

    if (load_sim(s->dispatch_op, s, s->conf.fmt, in, s->conf.load_addr))
        fatal(0, "Error while loading state into simulation");

    s->machine.regs[15] = s->conf.start_addr & PTR_MASK;

    struct run_ops ops = {
        .pre_insn = pre_insn,
        .post_insn = post_insn,
    };

    // XXX move run_debugger to its own header file once there is something
    // meaty to put there
    int run_debugger(struct sim_state *s, FILE *stream);
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

