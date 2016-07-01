#include "os_common.h"

#include "asm.h"
#include "asmif.h"
#include "common.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "param.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>

static const char shortopts[] = "df:o:p:qsv" "hV";

static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },
    { "param"       , required_argument, NULL, 'p' },
    { "quiet"       ,       no_argument, NULL, 'q' },
    { "verbose"     ,       no_argument, NULL, 'v' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

#define version() "tas version " STR(BUILD_NAME)

static int format_has_output(const struct format *f)
{
    return !!f->out;
}

static int usage(const char *me)
{
    char format_list[256];
    make_format_list(format_has_output, tenyr_asm_formats_count, tenyr_asm_formats,
            sizeof format_list, format_list, ", ");

    printf("Usage: %s [ OPTIONS ] file [ file ... ] \n"
           "Options:\n"
           "  -d, --disassemble     disassemble (default is to assemble)\n"
           "  -f, --format=F        select output format (%s)\n"
           "  -o, --output=X        write output to filename X\n"
           "  -p, --param=X=Y       set parameter X to value Y\n"
           "  -q, --quiet           disable disassembly output comments\n"
           "  -v, --verbose         disable simplified disassembly output\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string `%s'\n"
           , me, format_list, version());

    return 0;
}

static int process_stream(struct param_state *params, const struct format *f,
    FILE *in, FILE *out, int flags)
{
    int rc = 0;

    int disassemble = flags & ASM_DISASSEMBLE;
    FILE *stream = disassemble ? in : out;
    void *ud = NULL;

    if (f->init(stream, params, &ud))
        fatal(PRINT_ERRNO, "Error during initialisation for format '%s'", f->name);

    if (disassemble) {
        // This output might be consumed by a tool that needs a line at a time
        setvbuf(out, NULL, _IOLBF, 0);
        rc = do_disassembly(in, out, f, ud, flags);
    } else {
        rc = do_assembly(in, out, f, ud);
    }

    if (!rc && f->emit)
        rc |= f->emit(stream, &ud);

    rc |= f->fini(stream, &ud);

    fflush(out);

    return rc;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    volatile int disassemble = 0;
    volatile int flags = 0;

    extern int os_preamble(struct param_state *);
    extern FILE *os_fopen(const char *, const char *);

    char * volatile outfname = NULL;
    FILE * volatile out = stdout;

    struct param_state *params = NULL;
    param_init(&params);

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        if (out != stdout)
            // Technically there is a race condition here ; we would like to be
            // able to remove a file by a stream connected to it, but there is
            // apparently no portable way to do this.
            remove(outfname);
        return EXIT_FAILURE;
    }

    const struct format *f = &tenyr_asm_formats[0];

    // Explicitly reset optind for cases where main() is called more than once
    // (emscripten)
    optind = 0;

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'd': disassemble = 1; break;
            case 'f': if (find_format(optarg, &f)) usage(argv[0]), exit(EXIT_FAILURE); break;
            case 'o': outfname = optarg; break;
            case 'p': param_add(params, optarg); break;
            case 'q': flags |= ASM_QUIET; break;
            case 'v': flags |= ASM_VERBOSE; break;

            case 'V': puts(version()); return EXIT_SUCCESS;
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc)
        fatal(DISPLAY_USAGE, "No input files specified on the command line");

    param_set(params, "assembling", &"1\0""0\0"[disassemble], 1, false, false);

    os_preamble(params);

    // TODO don't open output until input has been validated
    if (outfname)
        out = os_fopen(outfname, "wb");
    if (!out)
        fatal(PRINT_ERRNO, "Failed to open output file `%s'", outfname ? outfname : "<stdout>");

    for (int i = optind; i < argc; i++) {
        const char *infname = argv[i];

        FILE *in = NULL;

        if (!strcmp(infname, "-")) {
            in = stdin;
        } else {
            in = os_fopen(infname, "rb");
            if (!in)
                fatal(PRINT_ERRNO, "Failed to open input file `%s'", infname);
        }

        // Explicitly clear errors and EOF in case we run main() twice
        // (emscripten)
        clearerr(in);

        if (disassemble)
            flags |= ASM_DISASSEMBLE;
        rc = process_stream(params, f, in, out, flags);
        fclose(in);

        if (rc)
            remove(outfname); // race condition ?
    }

    fclose(out);
    out = NULL;

    param_destroy(params);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
