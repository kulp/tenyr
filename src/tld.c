#include "obj.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>
#include <setjmp.h>

static const char shortopts[] = "o::hV";

static const struct option longopts[] = {
    { "output"      , required_argument, NULL, 'o' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

enum errcode { /* 0 impossible, 1 reserved for default */ DISPLAY_USAGE=2 };

static jmp_buf errbuf;

static void fatal(const char *message, enum errcode code)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    longjmp(errbuf, code);
}

static const char *version()
{
    return "tld version " STR(BUILD_NAME);
}

static int usage(const char *me)
{
    printf("Usage:\n"
           "  %s [ OPTIONS ] image-file [ image-file ... ] \n"
           "  -o, --output=X        write output to filename X\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print the string '%s'\n"
           , me, version());

    return 0;
}

int do_link(FILE *in, FILE *out)
{
    (void)in;
    (void)out;
    return -1;
}

int main(int argc, char *argv[])
{
    int rc = 0;

    FILE *out = stdout;

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        return EXIT_FAILURE;
    }

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'o': out = fopen(optarg, "w"); break;
            case 'V': puts(version()); return EXIT_SUCCESS;
            case 'h':
                usage(argv[0]);
                return EXIT_FAILURE;
            default:
                usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fatal("No input files specified on the command line", DISPLAY_USAGE);
    }

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
        if (!out) {
            perror("Failed to open output file");
            return EXIT_FAILURE;
        }

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "r");
            if (!in) {
                char buf[128];
                snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[i]);
                perror(buf);
                return EXIT_FAILURE;
            }
        }

        do_link(in, out);

        fclose(in);
    }

    fclose(out);

    return rc;
}

