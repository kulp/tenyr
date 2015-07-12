#include "os_common.h"

#include "asm.h"
#include "asmif.h"
#include "common.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"
#include "param.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>

#if _WIN32
#include <fcntl.h>
#include <io.h>
#endif

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
           "  -q, --quiet           disable disassembly output comments\n"
           "  -v, --verbose         disable simplified disassembly output\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string `%s'\n"
           , me, format_list, version());

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    volatile int disassemble = 0;
    volatile int flags = 0;

    volatile int opened = 0;
    char outfname[1044];
    FILE * volatile out = stdout;

    struct param_state *params = NULL;
    param_init(&params);

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        if (opened && out)
            // Technically there is a race condition here ; we would like to be
            // able to remove a file by a stream connected to it, but there is
            // apparently no portable way to do this.
            remove(outfname);
        return EXIT_FAILURE;
    }

    const struct format *f = &tenyr_asm_formats[0];

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'd': disassemble = 1; break;
            case 'f': if (find_format(optarg, &f)) usage(argv[0]), exit(EXIT_FAILURE); break;
            case 'o': out = fopen(strncpy(outfname, optarg, sizeof outfname - 1), "wb"); opened = 1; break;
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

#if _WIN32
    if (!disassemble)
        // ensure we are in binary mode on Windows
        if (out == stdout && setmode(1, O_BINARY) == -1)
            fatal(0, "Failed to set binary mode on stdout ; use -ofilename to avoid corrupted binaries.");
#endif

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
        if (!out)
            fatal(PRINT_ERRNO, "Failed to open output file");

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "rb");
            if (!in)
                fatal(PRINT_ERRNO, "Failed to open input file `%s'", argv[i]);
        }

        param_set(params, "assembling", (int[]){ !disassemble }, 1, false, false);
        void *ud = NULL;
        FILE *stream = disassemble ? in : out;
        if (f->init)
            if (f->init(stream, params, &ud))
                fatal(0, "Error during initialisation for format '%s'", f->name);

        if (disassemble) {
            // This output might be consumed by a tool that needs a line at a time
            setvbuf(out, NULL, _IOLBF, 0);
            if (f->in) {
                rc = do_disassembly(in, out, f, ud, flags);
            } else {
                fatal(0, "Format `%s' does not support disassembly", f->name);
            }
        } else {
            if (f->out) {
                rc = do_assembly(in, out, f, ud);
            } else {
                fatal(0, "Format `%s' does not support assembly", f->name);
            }
        }

        if (!rc && f->emit)
            rc |= f->emit(stream, &ud);
        else if (rc)
            remove(outfname); // race condition ?

        if (f->fini)
            rc |= f->fini(stream, &ud);

        fflush(out);

        fclose(in);
    }

    fclose(out);
    out = NULL;

    param_destroy(params);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
