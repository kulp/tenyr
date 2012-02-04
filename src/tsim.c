#include "ops.h"
#include "common.h"
#include "asm.h"
#include "device.h"
#include "sim.h"
// for RAM_BASE
#include "devices/ram.h"

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
    _(nowrap  , "stop when PC wraps around 24-bit boundary")

#define DEFAULT_RECIPES(_) \
    _(sparse) \
    _(serial) \
    _(nowrap)

#define Space(X) STR(X) " "

#define UsageDesc(Name,Desc) \
    "  " #Name ": " Desc "\n"

static int next_device(struct state *s)
{
    if (s->machine.devices_count >= s->machine.devices_max) {
        s->machine.devices_max *= 2;
        s->machine.devices = realloc(s->machine.devices,
                s->machine.devices_max * sizeof *s->machine.devices);
    }

    return s->machine.devices_count++;
}

static int recipe_abort(struct state *s)
{
    s->conf.abort = 1;
    return 0;
}

static int recipe_prealloc(struct state *s)
{
    int ram_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return ram_add_device(&s->machine.devices[index]);
}

static int recipe_sparse(struct state *s)
{
    int sparseram_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return sparseram_add_device(&s->machine.devices[index]);
}

static int recipe_serial(struct state *s)
{
    int serial_add_device(struct device **device);
    int index = next_device(s);
    s->machine.devices[index] = malloc(sizeof *s->machine.devices[index]);
    return serial_add_device(&s->machine.devices[index]);
}

static int recipe_nowrap(struct state *s)
{
    s->conf.nowrap = 1;
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

static int dispatch_op(struct state *s, int op, uint32_t addr, uint32_t *data)
{
    size_t count = s->machine.devices_count;
    struct device **device = bsearch(&addr, s->machine.devices, count, sizeof *device,
            find_device_by_addr);
    assert(("Found device to handle given address", device != NULL && *device != NULL));
    // TODO don't send in the whole simulator state ? the op should have
    // access to some state, in order to redispatch and potentially use other
    // machine.devices, but it shouldn't see the whole state
    return (*device)->op(s, (*device)->cookie, op, addr, data);
}

static const char shortopts[] = "a:s:f:nr:vhV";

static const struct option longopts[] = {
    { "address"    , required_argument, NULL, 'a' },
    { "format"     , required_argument, NULL, 'f' },
    { "scratch"    ,       no_argument, NULL, 'n' },
    { "recipe"     , required_argument, NULL, 'r' },
    { "verbose"    ,       no_argument, NULL, 'v' },

    { "help"       ,       no_argument, NULL, 'h' },
    { "version"    ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

static const char *version()
{
    return "tsim version " STR(BUILD_NAME);
}

static int usage(const char *me)
{
    printf("Usage:\n"
           "  %s [ OPTIONS ] imagefile\n"
           "  -a, --address=N       load instructions into memory at word address N\n"
           "  -s, --start=N         start execution at word address N\n"
           "  -f, --format=F        select input format (binary, text, obj)\n"
           "  -n, --scratch         don't run default recipes\n"
           "  -r, --recipe=R        run recipe R (see list below)\n"
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
           , me, version());

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

static int devices_setup(struct state *s)
{
    s->machine.devices_count = 0;
    s->machine.devices_max = 8;
    s->machine.devices = malloc(s->machine.devices_max * sizeof *s->machine.devices);

    return 0;
}

static int devices_finalise(struct state *s)
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
            s->machine.devices[i]->init(s, &s->machine.devices[i]->cookie);

    return 0;
}

static int devices_teardown(struct state *s)
{
    for (unsigned i = 0; i < s->machine.devices_count; i++) {
        s->machine.devices[i]->fini(s, s->machine.devices[i]->cookie);
        free(s->machine.devices[i]);
    }

    free(s->machine.devices);

    return 0;
}

static int run_recipe(struct state *s, recipe r)
{
    return r(s);
}

static int run_recipes(struct state *s)
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

static int add_recipe(struct state *s, const char *name)
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

static int run_sim(struct state *s)
{
    while (1) {
        assert(("PC within address space", !(s->machine.regs[15] & ~PTR_MASK)));
        // TODO make it possible to cast memory location to instruction again
        struct instruction i;
        s->dispatch_op(s, OP_READ, s->machine.regs[15], &i.u.word);

        if (s->conf.verbose > 0)
            printf("IP = 0x%06x\t", s->machine.regs[15]);
        if (s->conf.verbose > 1)
            print_disassembly(stdout, &i);
        if (s->conf.verbose > 3)
            fputs("\n", stdout);
        if (s->conf.verbose > 3)
            print_registers(stdout, s->machine.regs);
        if (s->conf.verbose > 0)
            fputs("\n", stdout);

        if (run_instruction(s, &i))
            return 1;
    }
}

static int load_sim(struct state *s, const struct format *f, FILE *in,
        int load_address, int start_address)
{
    devices_setup(s);
    run_recipes(s);
    devices_finalise(s);

    void *ud;
    if (f->init)
        f->init(in, ASM_DISASSEMBLE, &ud);

    struct instruction i;
    while (f->in(in, &i, ud) > 0) {
        s->dispatch_op(s, OP_WRITE, load_address++, &i.u.word);
    }

    if (f->fini)
        f->fini(in, &ud);

    s->machine.regs[15] = start_address & PTR_MASK;
    return 0;
}

int main(int argc, char *argv[])
{
    int rc = EXIT_SUCCESS;

    struct state _s = {
        .conf = {
            .verbose = 0,
            .run_defaults = 1,
        },
        .dispatch_op = dispatch_op,
    }, *s = &_s;

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        return EXIT_FAILURE;
    }

    int load_address = RAM_BASE, start_address = RAM_BASE;

    const struct format *f = &formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'a': load_address = strtol(optarg, NULL, 0); break;
            case 's': start_address = strtol(optarg, NULL, 0); break;
            case 'f': {
                size_t sz = formats_count;
                f = lfind(&(struct format){ .name = optarg }, formats, &sz,
                        sizeof formats[0], find_format_by_name);
                if (!f)
                    exit(usage(argv[0]));

                break;
            }
            case 'n': s->conf.run_defaults = 0; break;
            case 'r': add_recipe(s, optarg); break;
            case 'v': s->conf.verbose++; break;

            case 'V': puts(version()); return EXIT_SUCCESS;
            case 'h':
                usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fatal(DISPLAY_USAGE, "No input files specified on the command line");
    } else if (argc - optind > 1) {
        fatal(DISPLAY_USAGE, "More than one input file specified on the command line");
    }

    FILE *in = stdin;

    if (!strcmp(argv[optind], "-")) {
        in = stdin;
    } else {
        in = fopen(argv[optind], "r");
        if (!in) {
            char buf[128];
            snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[optind]);
            fatal(PRINT_ERRNO, buf);
        }
    }

    load_sim(s, f, in, load_address, start_address);
    run_sim(s);

    if (in)
        fclose(in);

    devices_teardown(s);

    return rc;
}

