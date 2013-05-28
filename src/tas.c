#include "asm.h"
#include "asmif.h"
#include "common.h"
#include "parser.h"
#include "parser_global.h"
#include "lexer.h"

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

static const char shortopts[] = "df:o:qsv" "hV";

static const struct option longopts[] = {
    { "disassemble" ,       no_argument, NULL, 'd' },
    { "format"      , required_argument, NULL, 'f' },
    { "output"      , required_argument, NULL, 'o' },
    { "quiet"       ,       no_argument, NULL, 'q' },
    { "strict"      ,       no_argument, NULL, 's' },
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
           "  -s, --strict          disable syntax sugar in disassembly\n"
           "  -v, --verbose         disable simplified disassembly output\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string `%s'\n"
           , me, format_list, version());

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    int disassemble = 0;
    int flags = 0;

    int opened = 0;
    char outfname[1044];
    FILE * volatile out = stdout;

    const struct format *f = &tenyr_asm_formats[0];

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

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'o': out = fopen(strncpy(outfname, optarg, sizeof outfname), "wb"); opened = 1; break;
            case 'd': disassemble = 1; break;
            case 'q': flags |= ASM_QUIET; break;
            case 's': flags |= ASM_NO_SUGAR; break;
            case 'v': flags |= ASM_VERBOSE; break;
            case 'f': {
                size_t sz = tenyr_asm_formats_count;
                f = lfind(&(struct format){ .name = optarg }, tenyr_asm_formats, &sz,
                        sizeof tenyr_asm_formats[0], find_format_by_name);
                if (!f)
                    exit(usage(argv[0]));

                break;
            }
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

    // ensure we are in binary mode on Windows
    if (out == stdout && freopen(NULL, "wb", stdout) == 0)
#if _WIN32
        if (setmode(1, O_BINARY) == -1)
#endif
            fatal(0, "Failed to set binary mode on stdout ; use -ofilename to avoid corrupted binaries.");

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
        if (!out)
            fatal(PRINT_ERRNO, "Failed to open output file");

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "rb");
            if (!in) {
                char buf[128];
                snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[i]);
                fatal(PRINT_ERRNO, buf);
            }
        }

        if (disassemble) {
            if (f->in) {
                rc = do_disassembly(in, out, f, flags);
            } else {
                fatal(0, "Format `%s' does not support disassembly", f->name);
            }
        } else {
            if (f->out) {
                rc = do_assembly(in, out, f);
            } else {
                fatal(0, "Format `%s' does not support assembly", f->name);
            }
        }

        fclose(in);
    }

    fclose(out);
    out = NULL;

    return rc;
}

