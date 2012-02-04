#include "obj.h"
// for RAM_BASE
#include "devices/ram.h"
#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>

struct defn {
    char name[LABEL_LEN];
    struct obj *obj;
    UWord reladdr;
};

struct link_state {
    UWord addr;     ///< current address
    int obj_count;
    struct obj_list {
        struct obj *obj;
        struct obj_list *next;
    } *objs;
    struct obj *relocated;
    void *defns;    ///< tsearch tree of struct defns

    long insns, syms, rlcs, words;
};

static const char shortopts[] = "o::hV";

static const struct option longopts[] = {
    { "output"      , required_argument, NULL, 'o' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

#define version() "tld version " STR(BUILD_NAME)

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

static int do_load(struct link_state *s, FILE *in)
{
    int rc = 0;
    struct obj_list *node = calloc(1, sizeof *node);

    struct obj *o = calloc(1, sizeof *o);
    size_t size;
    rc = obj_read(o, &size, in);

    // TODO this pushes the objects onto a stack, essentially ; should object
    // processing order matter, revisit this
    node->obj = o;
    node->next = s->objs;
    s->objs = node;
    s->obj_count++;

    return rc;
}

static int do_make_relocated(struct link_state *s, struct obj **_o)
{
    struct obj *o = *_o = calloc(1, sizeof *o);

    o->sym_count = 32;
    o->symbols = calloc(o->sym_count, sizeof *o->records);

    o->rlc_count = 32;
    o->relocs = calloc(o->rlc_count, sizeof *o->relocs);

    s->relocated = o;

    return 0;
}

static int do_link(struct link_state *s)
{
    int rc = -1;

    struct obj *o;
    do_make_relocated(s, &o);

    typedef int cmp(const void *, const void*);

    // read in all symbols
    list_foreach(obj_list, Node, s->objs) {
        if (Node->obj->sym_count) list_foreach(objsym, sym, Node->obj->symbols) {
            struct defn *def = calloc(1, sizeof *def);
            strncpy(def->name, sym->name, sizeof def->name);
            def->name[sizeof def->name - 1] = 0;
            def->obj = Node->obj;
            def->reladdr = sym->value;

            struct defn **look = tsearch(def, &s->defns, (cmp*)strcmp);
            if (*look != def)
                fatal(0, "Duplicate definition for symbol `%s'", def->name);
        }
    }

    // iterate over relocs
    list_foreach(obj_list, Node, s->objs) {
        struct obj *i = Node->obj;

        if (i->rlc_count) list_foreach(objrlc, rlc, i->relocs) {
            struct defn def;
            strncpy(def.name, rlc->name, sizeof def.name);
            def.name[sizeof def.name - 1] = 0;
            struct defn **look = tfind(&def, &s->defns, (cmp*)strcmp);
            if (!look)
                fatal(0, "Missing definition for symbol `%s'", rlc->name);
            // here we actually add the found-symbol's value to the relocation
            // slot, being careful to trim to the right width
            // XXX stop assuming there is only one record per object
            UWord *dest = &i->records->data[rlc->addr - i->records->addr] ;
            UWord mask = ((1 << rlc->width) - 1);
            UWord updated = (*dest + (*look)->reladdr) & mask;
            *dest = (*dest & ~mask) | updated;
        }
    }

    // TODO clean up symbols tree

    long rec_count = 0;
    // copy records
    list_foreach(obj_list, Node, s->objs) {
        struct obj *i = Node->obj;

        list_foreach(objrec, rec, i->records) {
            struct objrec *n = calloc(1, sizeof *n);

            n->addr = rec->addr;
            n->size = rec->size;
            n->data = malloc(rec->size * sizeof *n->data);
            memcpy(n->data, rec->data, rec->size * sizeof *n->data);

            if (o->records) o->records->prev = n;
            n->next = o->records;
            o->records = n;

            rec_count++;
        }
    }

    // TODO
    o->rec_count = rec_count;
    o->sym_count = s->syms;
    o->rlc_count = s->rlcs;
    o->length = 5 + s->words; // XXX explain
    o->records->size = s->addr;

    o->symbols = realloc(o->symbols, o->sym_count * sizeof *o->symbols);
    o->relocs  = realloc(o->relocs , o->rlc_count * sizeof *o->relocs);

    return rc;
}

static int do_emit(struct link_state *s, FILE *out)
{
    int rc = -1;

    if (s->relocated)
        rc = obj_write(s->relocated, out);

    return rc;
}

int main(int argc, char *argv[])
{
    int rc = 0;

    struct link_state _s = {
        .addr = 0,
    }, *s = &_s;

    if ((rc = setjmp(errbuf))) {
        if (rc == DISPLAY_USAGE)
            usage(argv[0]);
        return EXIT_FAILURE;
    }

    FILE *out = stdout;

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
        fatal(DISPLAY_USAGE, "No input files specified on the command line");
    }

    for (int i = optind; i < argc; i++) {
        FILE *in = NULL;
        if (!out)
            fatal(PRINT_ERRNO, "Failed to open output file");

        if (!strcmp(argv[i], "-")) {
            in = stdin;
        } else {
            in = fopen(argv[i], "r");
            if (!in) {
                char buf[128];
                snprintf(buf, sizeof buf, "Failed to open input file `%s'", argv[i]);
                fatal(PRINT_ERRNO, buf);
            }
        }

        do_load(s, in);

        fclose(in);
    }

    do_link(s);
    do_emit(s, out);

    fclose(out);

    return rc;
}

