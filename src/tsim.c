#include "os_common.h"
#include "plugin.h"

#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"
#include "stream.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <search.h>
#include <inttypes.h>
#include <stdbool.h>

int recipe_emscript(struct sim_state *s); // linked in externally

#define RECIPES(_) \
    _(emscript, "change behaviour to use an event loop for emscripten") \
    _(jit     , "use a JIT compiler (usually faster, but no -v supported)") \
    _(plugin  , "load plugins specified through param mechanism") \
    _(prealloc, "preallocate memory (higher memory footprint, maybe faster)") \
    _(serial  , "enable simple serial device and connect to stdio") \
    _(sparse  , "use sparse memory (lower memory footprint, maybe slower)") \
    _(top_page, "map a page at the highest addresses in memory") \
    _(tsimrc  , "parse tsimrc, after command-line args") \
    //

#define DEFAULT_RECIPES(_) \
    _(tsimrc)   \
    _(sparse)   \
    _(serial)   \
    _(plugin)   \
    _(top_page) \
    //

#define Space(X) STR(X) " "

#define UsageDesc(Name,Desc) \
    "  " #Name ": " Desc "\n"

struct sim_state;

typedef int recipe(struct sim_state *s);

struct recipe_book {
    recipe *recipe;
    const char *name;
    struct recipe_book *next;
};

struct library_list {
    struct library_list *next;
    void *handle;
};

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

static const char *library_search_paths[] = {
    ".." PATH_COMPONENT_SEPARATOR_STR "lib" PATH_COMPONENT_SEPARATOR_STR,
    "." PATH_COMPONENT_SEPARATOR_STR,
    "",
    NULL
};

static const char *version(void)
{
    return "tsim version " STR(BUILD_NAME) " built " __DATE__;
}

static int format_has_input(const struct format *f)
{
    return !!f->in;
}

static int usage(const char *me, int rc)
{
    char format_list[256];
    make_format_list(format_has_input, tenyr_asm_formats_count,
            tenyr_asm_formats, sizeof format_list, format_list, ", ");

    printf("Usage: %s [ OPTIONS ] image-file\n"
           "Options:\n"
           "  -@, --options=X       use options from file X\n"
           "  -a, --address=N       load instructions into memory at word address N\n"
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
           "Available recipes (in alphabetical order):\n"
           RECIPES(UsageDesc)
           "Default recipes (in order of execution):\n"
           "  " DEFAULT_RECIPES(Space)
           "\n"
           , me, format_list, version());

    return rc;
}

static int pre_fetch(struct sim_state *s, void *ud)
{
    (void)ud;
    if (s->machine.regs[15] == s->conf.halt_addr)
        return 1;

    return 0;
}

static int pre_insn(struct sim_state *s, const struct element *i, void *ud)
{
    (void)ud;
    if (s->conf.verbose > 0)
        fprintf(stderr, "IP = 0x%08x\t", s->machine.regs[15]);

    const struct stream out_ = stream_make_from_file(stderr), *out = &out_;

    if (s->conf.verbose > 1) {
        int len = print_disassembly(out, i, ASM_AS_INSN);
        fprintf(stderr, "%*s# ", 30 - len, "");
        print_disassembly(out, i, ASM_AS_DATA);
    }

    if (s->conf.verbose > 3) {
        fwrite("\n", 1, 1, stderr);
        print_registers(out, s->machine.regs);
    }

    if (s->conf.verbose > 0)
        fwrite("\n", 1, 1, stderr);

    return 0;
}

static int post_insn(struct sim_state *s, const struct element *i, void *ud)
{
    (void)i;
    (void)ud;
    s->insns_executed++;
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
    if (!ptr) {
        debug(0, "Failed to find symbol `%s` - %s", buf, dlerror());
        return 1;
    }

    typedef int add_device(struct device *);
    add_device *adder = ALIASING_CAST(add_device,ptr);
    rc |= adder(new_device(s));

    return rc;
}

static int recipe_plugin(struct sim_state *s)
{
    int result = s->plugins_loaded ||
        (s->plugins_loaded = !plugin_load(s->conf.tsim_path, library_search_paths, "plugin", &s->plugin_cookie, plugin_success, s));
    return !result;
}

static int recipe_runner(struct sim_state *s, const char *prefix)
{
    const char **path = library_search_paths;
    void *libhandle = NULL;
    char *error = "(no error)";
    while ((libhandle == NULL) && *path != NULL) {
        char *buf = build_path(s->conf.tsim_path, "%slibtenyr%s"DYLIB_SUFFIX, *path++, prefix);
        void *handle = dlopen(buf, RTLD_NOW | RTLD_LOCAL);
        error = dlerror();
        if (handle)
            libhandle = handle;
        else
            debug(1, "Did not load library `%s`: %s", buf, error);
        free(buf);
    }
    if (libhandle == NULL) {
        debug(0, "Failed to load `%s` library: %s", prefix, error);
        return 1;
    }

    char name[128];
    if (snprintf(name, sizeof name, "%s_run_sim", prefix) >= (long)sizeof name) {
        debug(0, "Library name prefix too long");
        if (libhandle != NULL)
            dlclose(libhandle);
        return 1;
    }

    void *ptr = dlsym(libhandle, name);
    if (!ptr) {
        debug(0, "Failed to find symbol `%s` - %s", name, dlerror());
        if (libhandle != NULL)
            dlclose(libhandle);
        return 1;
    }

    struct library_list *t = malloc(sizeof *t);
    t->next = s->libs;
    t->handle = libhandle;
    s->libs = t;

    s->run_sim = ALIASING_CAST(sim_runner, ptr);

    return 0;
}

static int recipe_jit(struct sim_state *s)
{
    return recipe_runner(s, "jit");
}

static int parse_opts_file(struct sim_state *s, const char *filename);

static int recipe_tsimrc(struct sim_state *s)
{
    size_t size = 128;
    char *path = NULL;
    int rc;
    do {
        path = realloc(path, size);
        rc = os_get_tsimrc_path(path, size);
        size *= 2;
    } while (rc && errno == ENOSPC);

    if (!rc)
        rc = parse_opts_file(s, path);

    free(path);

    // silently ignore lack of existence
    return (errno == ENOENT) ? 0 : rc;
}

static int recipe_top_page(struct sim_state *s)
{
    return ram_add_device_sized(new_device(s), -4096, 4096);
}

#define DEVICE_RECIPE_TMPL(Name,Func)                                          \
    static int recipe_##Name(struct sim_state *s)                              \
    {                                                                          \
        device_adder Func;                                                     \
        return Func(new_device(s));                                            \
    }                                                                          \
    //

DEVICE_RECIPE_TMPL(prealloc,      ram_add_device)
DEVICE_RECIPE_TMPL(sparse  ,sparseram_add_device)
DEVICE_RECIPE_TMPL(serial  ,   serial_add_device)

static int run_recipes(struct sim_state *s)
{
    if (s->conf.run_defaults) {
        #define RUN_RECIPE(Recipe) recipe_##Recipe(s);
        DEFAULT_RECIPES(RUN_RECIPE);
    }

    list_foreach(recipe_book, b, s->recipes_head) {
        if (b->recipe(s))
            fatal(PRINT_ERRNO, "Running recipe `%s` failed", b->name);
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
    lfind_size_t sz = countof(entries);

    struct recipe_entry *r = lfind(&(struct recipe_entry){ .name = name },
            entries, &sz, sizeof entries[0],
            find_recipe_by_name);

    if (r) {
        struct recipe_book *n = *s->recipes_tail = malloc(sizeof *n);
        n->next = NULL;
        n->recipe = r->recipe;
        n->name = r->name;
        s->recipes_tail = &n->next;

        return 0;
    } else {
        return 1;
    }
}

static int plugin_param_get(const struct plugin_cookie *cookie, const char *key, size_t count, const void **val)
{
    return param_get(cookie->param, key, count, val);
}

static int plugin_param_get_int(const struct plugin_cookie *cookie, const char *key, int *val)
{
    return param_get_int(cookie->param, key, val);
}

static int plugin_param_set(struct plugin_cookie *cookie, char *key, char *val, int replace, int free_key, int free_value)
{
    return param_set(cookie->param, key, val, replace, free_key, free_value);
}

static int parse_args(struct sim_state *s, int argc, char *argv[]);

static int parse_opts_file(struct sim_state *s, const char *filename)
{
    FILE *f = os_fopen(filename, "r");
    if (!f) {
        char *b = build_path(s->conf.tsim_path, "../share/tenyr/%s", filename);
        f = os_fopen(b, "r");
        free(b);
        if (!f)
            return 1;
    }

    char buf[1024], *p;
    while ((p = fgets(buf, sizeof buf, f))) {
        // trim newline
        size_t len = strlen(buf);
        if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

        // Split words by first space character found. This approximates, but
        // does not replicate, behaviour of the shell on the command line.
        // There doesn't need to be a space at all, in which case the second
        // word pointer is NULL.
        char *word0 = buf;
        char *word1 = NULL;

        // Update p to the end of the first word
        while (*p != '\0' && !isspace(*p)) {
            p++;
        }

        // If there are spaces, squash them and point word1 past them
        while (*p != '\0' &&  isspace(*p)) {
            *p++ = '\0';
            word1 = p;
        }

        int oi = optind;
        optind = 0;
        char *pbuf[] = { NULL, word0, word1, NULL };
        parse_args(s, 2 + (word1 != NULL), pbuf);
        optind = oi;
    }

    int e = ferror(f);
    fclose(f);

    return e;
}

static int parse_args(struct sim_state *s, int argc, char *argv[])
{
    // Explicitly reset optind for cases where main() is called more than once
    // (emscripten)
    optind = 0;

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case '@': if (parse_opts_file(s, optarg)) fatal(PRINT_ERRNO, "Error in opts file"); break;
            case 'a': s->conf.load_addr = (int32_t)strtol(optarg, NULL, 0); break;
            case 'd': s->conf.debugging = 1; break;
            case 'f': if (find_format(optarg, &s->conf.fmt)) exit(usage(argv[0], EXIT_FAILURE)); break;
            case 'n': s->conf.run_defaults = 0; break;
            case 'p': param_add(s->conf.params, optarg); break;
            case 'r': if (add_recipe(s, optarg)) exit(usage(argv[0], EXIT_FAILURE)); break;
            case 's': s->conf.start_addr = (int32_t)strtol(optarg, NULL, 0); break;
            case 'v': s->conf.verbose++; break;

            case 'V': puts(version()); exit(EXIT_SUCCESS);
            case 'h': exit(usage(argv[0], EXIT_SUCCESS));
            default : exit(usage(argv[0], EXIT_FAILURE));
        }
    }

    return 0;
}

static int read_sim_params(struct sim_state *s)
{
    {
        int truth = 0;
        param_get_int(s->conf.params, "tsim.continue_on_invalid_device", &truth);
        if (truth)
            s->conf.flags |= SIM_CONTINUE_ON_INVALID_DEVICE;
    }

    {
        int truth = 0;
        param_get_int(s->conf.params, "tsim.continue_on_failed_mem_op", &truth);
        if (truth)
            s->conf.flags |= SIM_CONTINUE_ON_FAILED_MEM_OP;
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
            .halt_addr    = (signed)0xffffffff, // default to legacy halt behavior
            .fmt          = &tenyr_asm_formats[0],
            .tsim_path    = os_find_self(argv[0]),
        },
        .dispatch_op = dispatch_op,
        .plugin_cookie = {
            .gops         = {
                .fatal = fatal_,
                .debug = debug_,
                .param_get = plugin_param_get,
                .param_get_int = plugin_param_get_int,
                .param_set = plugin_param_set,
            },
        },
        .run_sim      = interp_run_sim,
        .interp       = interp_run_sim,
        .pump         = devices_dispatch_cycle,
        .recipes_tail = &_s.recipes_head,
    }, *s = &_s;

    os_preamble();

    param_init(&s->conf.params);
    s->plugin_cookie.param = s->conf.params;

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0], EXIT_FAILURE);
        rc = EXIT_FAILURE;
        goto cleanup;
    }

    // Trailing slash is required, because `build_path` strips off last path
    // component (so that it works when passed a path to a file)
    char *share_path = build_path(s->conf.tsim_path, "../share/tenyr/");
    // Don't replace any existing share path
    param_set(s->conf.params, "paths.share", share_path, false, false, true);

    parse_args(s, argc, argv);

    if (optind >= argc) {
        fatal(DISPLAY_USAGE, "No input files specified on the command line");
    } else if (argc - optind > 1) {
        fatal(DISPLAY_USAGE, "More than one input file specified on the command line");
    }

    param_get_int(s->conf.params, "addrs.halt", &s->conf.halt_addr);

    FILE *infile;

    if (!strcmp(argv[optind], "-")) {
        infile = stdin;
    } else {
        infile = os_fopen(argv[optind], "rb");
        if (!infile)
            fatal(PRINT_ERRNO, "Failed to open input file `%s'", argv[optind]);
    }

    // Explicitly clear errors and EOF in case we run main() twice
    // (emscripten)
    clearerr(infile);

    const struct stream in_ = stream_make_from_file(infile), *in = &in_;

    devices_setup(s);
    run_recipes(s);
    if (devices_finalise(s))
        fatal(0, "Error while finalising devices setup");

    param_set(s->conf.params, "assembling", "0", true, false, false);
    void *ud = NULL;
    const struct format *f = s->conf.fmt;
    if (f->init(in, s->conf.params, &ud))
        fatal(0, "Error during initialisation for format '%s'", f->name);

    if (load_sim(s->dispatch_op, s, f, ud, in, s->conf.load_addr))
        fatal(0, "Error while loading state into simulation");

    f->fini(in, &ud);

    memset(s->machine.regs, 0x00, sizeof s->machine.regs);
    s->machine.regs[15] = s->conf.start_addr;

    struct run_ops ops = {
        .pre_fetch = pre_fetch,
        .pre_insn = pre_insn,
        .post_insn = post_insn,
    };

    read_sim_params(s);

    void *run_ud = NULL;
    rc = s->run_sim(s, &ops, &run_ud, NULL);
    if (rc < 0)
        fprintf(stderr, "Error during simulation, P=0x%08x\n", s->machine.regs[15]);

    fclose(infile);

    devices_teardown(s);

    if (s->conf.debugging > 0)
        fprintf(stderr, "Instructions executed: %lu\n", s->insns_executed);

cleanup:
    param_destroy(s->conf.params);

    while (s->libs) {
        struct library_list *t = s->libs;
        if (t->handle != NULL)
            dlclose(t->handle);
        s->libs = s->libs->next;
        free(t);
    }

    free(s->conf.tsim_path);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
