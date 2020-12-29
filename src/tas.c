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
#include <stdbool.h>

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

static const char *version(void)
{
    return "tas version " STR(BUILD_NAME) " built " __DATE__;
}

static int format_has_output(const struct format *f)
{
    return !!f->out;
}

static int usage(const char *me, int rc)
{
    char format_list[256] = { 0 };
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

    return rc;
}

static int process_stream(struct param_state *params, const struct format *f,
    FILE *infile, FILE *outfile, int flags)
{
    int rc = 0;

    const struct stream in_ = stream_make_from_file(infile), *in = &in_;
    const struct stream out_ = stream_make_from_file(outfile), *out = &out_;

    int disassemble = flags & ASM_DISASSEMBLE;
    STREAM *stream = disassemble ? in : out;
    void *ud = NULL;

    if (f->init(stream, params, &ud))
        fatal(PRINT_ERRNO, "Error during initialisation for format '%s'", f->name);

    if (disassemble) {
        // This output might be consumed by a tool that needs a line at a time
        os_set_buffering(outfile, _IOLBF);
        rc = do_disassembly(in, out, f, ud, flags);
    } else {
        rc = do_assembly(in, out, f, ud);
    }

    if (!rc && f->emit)
        rc |= f->emit(stream, &ud);

    rc |= f->fini(stream, &ud);

    out->op.fflush(out);

    return rc;
}

static int process_file(struct param_state *params, int flags,
        const struct format *fmt, const char *infname, FILE *out)
{
    int rc = 0;
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

    rc = process_stream(params, fmt, in, out, flags);
    fclose(in);

    return rc;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    volatile int disassemble = 0;
    volatile int flags = 0;

    char * volatile outfname = NULL;
    FILE * volatile out = stdout;

    struct param_state * volatile params = NULL;
    param_init((struct param_state **)&params);

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0], EXIT_FAILURE);
        // We may have created an output file already, but we do not try to
        // remove it, because doing so by filename would be a race condition.
        // The most important reason to remove the output file is to avoid
        // tricking a build system into thinking that a failed build created a
        // good output file; with GNU Make this can be avoided by using
        // .DELETE_ON_ERROR, and other build systems have similar features.
        rc = EXIT_FAILURE;
        goto cleanup;
    }

    const struct format *fmt = &tenyr_asm_formats[0];

    // Explicitly reset optind for cases where main() is called more than once
    // (emscripten)
    optind = 0;

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'd': disassemble = 1; break;
            case 'f': if (find_format(optarg, &fmt)) exit(usage(argv[0], EXIT_FAILURE)); break;
            case 'o': outfname = optarg; break;
            case 'p': param_add(params, optarg); break;
            case 'q': flags |= ASM_QUIET; break;
            case 'v': flags |= ASM_VERBOSE; break;

            case 'V':
                puts(version());
                rc = EXIT_SUCCESS;
                goto cleanup;
            case 'h':
                usage(argv[0], EXIT_SUCCESS);
                rc = EXIT_SUCCESS;
                goto cleanup;
            default:
                usage(argv[0], EXIT_FAILURE);
                rc = EXIT_FAILURE;
                goto cleanup;
        }
    }

    if (optind >= argc)
        fatal(DISPLAY_USAGE, "No input files specified on the command line");

    param_set(params, "assembling", &"1\0""0\0"[disassemble], 1, false, false);

    {
        int setting  = 0;
        int clearing = 0;

        // Return values from param_get_int are not significant in this case,
        // since we have correct defaults.
        param_get_int(params, "tas.flags.set"  , &setting );
        param_get_int(params, "tas.flags.clear", &clearing);

        flags = (flags | setting) & ~clearing;
    }

    os_preamble();

    // TODO don't open output until input has been validated
    if (outfname)
        out = os_fopen(outfname, "wb");
    if (!out)
        fatal(PRINT_ERRNO, "Failed to open output file `%s'", outfname ? outfname : "<stdout>");

    if (disassemble)
        flags |= ASM_DISASSEMBLE;

    for (int i = optind; i < argc; i++) {
        rc = process_file(params, flags, fmt, argv[i], out);
        // We may have created an output file already, but we do not try to
        // remove it, because doing so by filename would be a race condition.
        // The most important reason to remove the output file is to avoid
        // tricking a build system into thinking that a failed build created a
        // good output file; with GNU Make this can be avoided by using
        // .DELETE_ON_ERROR, and other build systems have similar features.
    }

    fclose(out);
    out = NULL;

cleanup:
    param_destroy(params);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
