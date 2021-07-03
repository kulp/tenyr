#define _XOPEN_SOURCE (700) /* for strdup */

#include "obj.h"
// for RAM_BASE
#include "devices/ram.h"
#include "common.h"
#include "os_common.h"
#include "param.h"

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <search.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <assert.h>

struct link_state {
    SWord addr;     ///< current address
    int obj_count;
    struct obj_list {
        struct obj *obj;
        int i;
        struct obj_list *next;
    } *objs, **next_obj;
    struct obj *relocated;

    long insns, syms, rlcs, words;
};

struct defn {
    char *name;
    struct obj *obj;
    SWord reladdr;
    SWord flags;

    struct link_state *state;   ///< state reference used for twalk() support
};

struct objmeta {
    struct obj *obj;
    int size;
    int offset;

    struct link_state *state;   ///< state reference used for twalk() support
};

static const char shortopts[] = "o:p:hV";

static const struct option longopts[] = {
    { "output"      , required_argument, NULL, 'o' },
    { "param"       , required_argument, NULL, 'p' },

    { "help"        ,       no_argument, NULL, 'h' },
    { "version"     ,       no_argument, NULL, 'V' },

    { NULL, 0, NULL, 0 },
};

static const char *version(void)
{
    return "tld version " STR(BUILD_NAME) " built " __DATE__;
}


static void usage(const char *me)
{
    printf("Usage: %s [ OPTIONS ] image-file [ image-file ... ] \n"
           "Options:\n"
           "  -o, --output=X        write output to filename X\n"
           "  -p, --param=X=Y       set parameter X to value Y\n"
           "  -h, --help            display this message\n"
           "  -V, --version         print a string describing the version\n"
           , me);
}

static int do_load(struct link_state *s, STREAM *in)
{
    int rc = 0;
    struct obj_list *node = calloc(1, sizeof *node);

    struct obj *o = calloc(1, sizeof *o);
    rc = obj_read(o, in);
    node->obj = o;
    node->i = s->obj_count++;
    // put the objects on the list in order
    node->next = NULL;
    *s->next_obj = node;
    s->next_obj = &node->next;

    return rc;
}

static void do_unload(struct link_state *s)
{
    list_foreach(obj_list,ol,s->objs) {
        obj_free(ol->obj);
        free(ol);
    }
}

static int ptrcmp(const void *a, const void *b)
{
    return (int)(*(const char* const*)a - *(const char* const*)b);
}

static int def_str_cmp(const void *a, const void *b)
{
    const struct defn *aa = a, *bb = b;
    return strcmp(aa->name, bb->name);
}

static void do_link_build_state(struct link_state *s, void **objtree, void **defns)
{
    // running offset, tracking where to pack objects tightly one after another
    SWord running = 0;

    // read in all symbols
    list_foreach(obj_list, Node, s->objs) {
        struct obj *i = Node->obj;

        // Loading an object has already validated the object count.
        assert(i->rec_count >= 0);
        assert(i->rec_count < OBJ_MAX_REC_CNT);

        if (i->rec_count == 0) {
            debug(1, "Object has no records, skipping");
            continue;
        }

        // TODO support more than one record per object
        if (i->rec_count > 1)
            fatal(0, "Object has more than one record (unsupported)");

        struct objmeta *meta = calloc(1, sizeof *meta);
        meta->state = s;
        meta->obj = i;
        meta->size = i->records[0].size;
        meta->offset = running;
        debug(1, "Object %p adds record size %d @ %#x", i, meta->size, meta->offset);
        running += i->records[0].size;

        tsearch(meta, objtree, ptrcmp);

        list_foreach(objsym, sym, i->symbols) {
            struct defn *def = calloc(1, sizeof *def);
            def->state = s;
            def->name = strdup(sym->name.str);
            def->obj = i;
            def->reladdr = sym->value;
            def->flags = sym->flags;
            debug(2, "Object %p adds symbol `%s` @ %#x", i, def->name, def->reladdr);

            struct defn **look = tsearch(def, defns, def_str_cmp);
            if (*look != def)
                fatal(0, "Duplicate definition for symbol `%s'", def->name);
        }
    }
}

static void do_link_relocate_obj_reloc(struct obj *i, struct objrlc *rlc,
                                       void **objtree, void **defns)
{
    SWord reladdr = 0;

    // Objects with record counts other than 1 have already been pruned in
    // do_link_build_state.
    // TODO support more than one record per object
    assert(i->rec_count == 1);

    struct objrec *r = &i->records[0];
    if (rlc->addr < r->addr ||
        rlc->addr - r->addr > r->size)
    {
        fatal(0, "Invalid relocation @ 0x%08x outside record @ 0x%08x size %d",
              rlc->addr, r->addr, r->size);
        return;
    }

    struct objmeta **me = tfind(&i, objtree, ptrcmp);
    if (rlc->name.len) { // TODO use length
        debug(1, "Object %p relocating name `%s` from %#x to %#x", i, rlc->name.str, rlc->addr, reladdr);
        struct defn def;
        def.name = rlc->name.str; // safe as long as we use tfind() but not tsearch()
        struct defn **look = tfind(&def, defns, def_str_cmp);
        if (!look)
            fatal(0, "Missing definition for symbol `%s'", rlc->name.str);
        reladdr = (*look)->reladdr;

        if (((*look)->flags & RLC_ABSOLUTE) == 0) {
            struct objmeta **it = tfind(&(*look)->obj, objtree, ptrcmp);
            reladdr += (*it)->offset;
        }
    } else {
        debug(1, "Object %p relocating . @ %#x", i, rlc->addr);
        // this is a null relocation ; it just wants us to update the offset
        reladdr = (*me)->offset;
    }
    // here we actually add the found-symbol's value to the relocation slot,
    // being careful to trim to the right width
    SWord mult = (rlc->flags & RLC_NEGATE) ? -1 : +1;
    SWord *dest = &r->data[rlc->addr - r->addr];
    SWord mask = ((SWord)((1u << (rlc->width - 1)) << 1) - 1);
    SWord updated = (*dest + mult * (reladdr >> rlc->shift)) & mask;
    *dest = (*dest & ~mask) | updated;
}

static void do_link_relocate_obj(struct obj *i, void **objtree, void **defns)
{
    list_foreach(objrlc, rlc, i->relocs)
        do_link_relocate_obj_reloc(i, rlc, objtree, defns);
}

static void do_link_relocate(struct obj_list *ol, void **objtree, void **defns)
{
    list_foreach(obj_list, Node, ol) {
        if (!Node->obj->rec_count) {
            debug(1, "Object has no records, skipping");
            continue;
        }

        do_link_relocate_obj(Node->obj, objtree, defns);
    }
}

static void do_link_process(struct link_state *s)
{
    void *objtree = NULL;   ///< tsearch-tree of `struct objmeta'
    void *defns   = NULL;   ///< tsearch tree of `struct defns'

    do_link_build_state(s, &objtree, &defns);
    do_link_relocate(s->objs, &objtree, &defns);

    while (objtree) {
        void *node = *(void**)objtree;
        tdelete(node, &objtree, ptrcmp);
        free(node);
    }

    while (defns) {
        struct defn *node = *(void**)defns;
        tdelete(node, &defns, def_str_cmp);
        free(node->name);
        free(node);
    }
}

static void do_link_emit(struct link_state *s, struct obj *o)
{
    long rec_count = 0;
    // copy records
    struct objrec **ptr_objrec = &o->records, *front = NULL;
    int32_t addr = 0;
    list_foreach(obj_list, Node, s->objs) {
        struct obj *i = Node->obj;

        list_foreach(objrec, rec, i->records) {
            struct objrec *n = calloc(1, sizeof *n);

            n->addr = addr;
            n->size = rec->size;
            n->data = malloc((size_t)rec->size * sizeof *n->data);
            n->next = NULL;
            memcpy(n->data, rec->data, (size_t)rec->size * sizeof *n->data);

            if (*ptr_objrec) (*ptr_objrec)->next = n;
            if (!front) front = n;
            *ptr_objrec = n;
            ptr_objrec = &n->next;

            addr += rec->size;
            rec_count++;
        }
    }

    o->records = front;

    o->rec_count = (SWord)rec_count;
    o->sym_count = (SWord)s->syms;
    o->rlc_count = (SWord)s->rlcs;
}

static void do_link(struct link_state *s)
{
    struct obj *o = s->relocated = calloc(1, sizeof *o);

    do_link_process(s);
    do_link_emit(s, o);
}

static void do_emit(struct link_state *s, STREAM *out)
{
    obj_write(s->relocated, out);
}

static int do_load_all(struct link_state *s, int count, char **names)
{
    int rc = 0;

    for (int i = 0; i < count; i++) {
        FILE *infile = NULL;

        if (!strcmp(names[i], "-")) {
            infile = stdin;
        } else {
            infile = os_fopen(names[i], "rb");
            if (!infile)
                fatal(PRINT_ERRNO, "Failed to open input file `%s'", names[i]);
        }

        // Explicitly clear errors and EOF in case we run main() twice
        // (emscripten)
        clearerr(infile);

        const struct stream in_ = stream_make_from_file(infile), *in = &in_;

        rc = do_load(s, in);
        int saved = errno;

        fclose(infile);

        errno = saved;
        if (rc)
            return rc;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int rc = 0;

    struct link_state _s = {
        .addr = 0,
        .next_obj = &_s.objs,
    }, *s = &_s;

    char * volatile outfname = NULL;
    FILE * volatile outfile = stdout;

    struct param_state * volatile params = NULL;
    param_init((struct param_state **)&params);

    if ((rc = setjmp(errbuf))) {
        if (rc & DISPLAY_USAGE)
            usage(argv[0]);
        // We may have created an output file already, but we do not try to
        // remove it, because doing so by filename would be a race condition.
        // The most important reason to remove the output file is to avoid
        // tricking a build system into thinking that a failed build created a
        // good output file; with GNU Make this can be avoided by using
        // .DELETE_ON_ERROR, and other build systems have similar features.
        rc = EXIT_FAILURE;
        goto cleanup;
    }

    // Explicitly reset optind for cases where main() is called more than once
    // (emscripten)
    optind = 0;

    int ch;
    while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
        switch (ch) {
            case 'o': outfname = optarg; break;
            case 'p': param_add(params, optarg); break;

            case 'V':
                puts(version());
                rc = EXIT_SUCCESS;
                goto cleanup;
            case 'h':
                usage(argv[0]);
                rc = EXIT_SUCCESS;
                goto cleanup;
            default:
                usage(argv[0]);
                rc = EXIT_FAILURE;
                goto cleanup;
        }
    }

    if (optind >= argc)
        fatal(DISPLAY_USAGE, "No input files specified on the command line");

    os_preamble();

    if (outfname)
        outfile = os_fopen(outfname, "wb");
    if (!outfile)
        fatal(PRINT_ERRNO, "Failed to open output file `%s'", outfname ? outfname : "<stdout>");

    const struct stream out_ = stream_make_from_file(outfile), *out = &out_;

    rc = do_load_all(s, argc - optind, &argv[optind]);
    if (rc)
        fatal(PRINT_ERRNO, "Failed to load objects");
    do_link(s);
    do_emit(s, out);
    do_unload(s);
    list_foreach(objrec, rec, s->relocated->records) {
        free(rec->data);
        free(rec);
    }
    free(s->relocated);

    fclose(outfile);
    out = NULL;

cleanup:
    param_destroy(params);

    return rc;
}

/* vi: set ts=4 sw=4 et: */
