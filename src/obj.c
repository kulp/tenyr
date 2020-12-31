// TODO make an mmap()-based version of this for systems that support it

// `context` parameters not often used in obj_op
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "obj.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <assert.h>

#define MAGIC_BYTES "TOV"
#define OBJ_DEFAULT_VERSION 2

#define PUT(What,Where) put_sized(&(What), sizeof (What), 1, Where)
#define GET(What,Where) get_sized(&(What), sizeof (What), 1, Where)

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define get_sized get_sized_le
#define put_sized put_sized_le
#else
#define get_sized get_sized_be
#define put_sized put_sized_be
#endif

typedef int obj_op(struct obj *o, STREAM *out, void *context);

static obj_op
    put_recs     , get_recs     ,
    put_syms_v2  , get_syms_v2  ,
    put_relocs_v2, get_relocs_v2;

static const struct objops {
    obj_op *put_recs, *put_syms, *put_relocs,
           *get_recs, *get_syms, *get_relocs;
} objops = {
    .put_recs = put_recs, .put_syms = put_syms_v2, .put_relocs = put_relocs_v2,
    .get_recs = get_recs, .get_syms = get_syms_v2, .get_relocs = get_relocs_v2,
};

static inline void get_sized_le(void *what, size_t size, size_t count, STREAM *where)
{
    if (size * count == 0) {
        return;
    }
    if (where->op.fread(what, size, count, where) != count) {
        if (where->op.feof(where)) {
            fatal(0, "End of file unexpectedly reached while parsing object");
        } else {
            fatal(0, "Unknown error while parsing object");
        }
    }
}

static inline void put_sized_le(const void *what, size_t size, size_t count, STREAM *where)
{
    if (size * count == 0) {
        return;
    }
    if (where->op.fwrite(what, size, count, where) != count) {
        if (where->op.feof(where)) {
            fatal(0, "End of file unexpectedly reached while emitting object");
        } else {
            fatal(0, "Unknown error while emitting object");
        }
    }
}

static inline int32_t swapword(const int32_t in)
{
    return (((in >> 24) & 0xff) <<  0) |
           (((in >> 16) & 0xff) <<  8) |
           (((in >>  8) & 0xff) << 16) |
           (((in >>  0) & 0xff) << 24);
}

static inline void get_sized_be(void *what, size_t size, size_t count, STREAM *where)
{
    get_sized_le(what, size, count, where);
    // get_sized() isn't as general as it looks - it does bytes, UWords, and
    // strings.
    if (size == sizeof(SWord)) {
        SWord *dest = what;
        for (size_t i = 0; i < count; i++)
            dest[i] = swapword(dest[i]);
    }
}

static inline void put_sized_be(const void *what, size_t size, size_t count, STREAM *where)
{
    // put_sized() has an analagous caveat to get_sized()'s, but we only swap
    // one word at a time so we don't have to allocate arbitrarily-large
    // buffers.
    if (size == sizeof(SWord)) {
        const SWord *src = what;
        for (size_t i = 0; i < count; i++) {
            const SWord temp = swapword(src[i]);
            put_sized_le(&temp, sizeof(SWord), 1, where);
        }
    } else {
        put_sized_le(what, size, count, where);
    }
}

static int put_recs(struct obj *o, STREAM *out, void *context)
{
    PUT(o->rec_count, out);
    list_foreach(objrec, rec, o->records) {
        PUT(rec->addr, out);
        PUT(rec->size, out);
        put_sized(rec->data, sizeof *rec->data, (size_t)rec->size, out);
    }

    return 0;
}

static int put_syms_v2(struct obj *o, STREAM *out, void *context)
{
    PUT(o->sym_count, out);
    list_foreach(objsym, sym, o->symbols) {
        PUT(sym->flags, out);
        PUT(sym->name.len, out);
        put_sized(sym->name.str, round_up_to_word((size_t)sym->name.len), 1, out);
        PUT(sym->value, out);
        PUT(sym->size, out);
    }

    return 0;
}

static int put_relocs_v2(struct obj *o, STREAM *out, void *context)
{
    PUT(o->rlc_count, out);
    list_foreach(objrlc, rlc, o->relocs) {
        PUT(rlc->flags, out);
        PUT(rlc->name.len, out);
        put_sized(rlc->name.str, round_up_to_word((size_t)rlc->name.len), 1, out);
        PUT(rlc->addr, out);
        PUT(rlc->width, out);
        PUT(rlc->shift, out);
    }

    return 0;
}

static int obj_v2_write(struct obj *o, STREAM *out, const struct objops *ops)
{
    put_sized(MAGIC_BYTES, 3, 1, out);
    PUT(o->magic.parsed.version, out);
    PUT(o->flags, out);

    {
        int rc = 0;
        rc = ops->put_recs  (o, out, NULL); if (rc) return rc;
        rc = ops->put_syms  (o, out, NULL); if (rc) return rc;
        rc = ops->put_relocs(o, out, NULL); if (rc) return rc;
    }

    return 0;
}

int obj_write(struct obj *o, STREAM *out)
{
    int version = o->magic.parsed.version = OBJ_DEFAULT_VERSION;

    switch (version) {
        case 2:
            return obj_v2_write(o, out, &objops);
        default:
            fatal(0, "Unhandled version %d while emitting object", version);
    }
}

// CREATE_SCOPE uses a C99 for-loop to create a variable whose lifetime is only
// the statement following the CREATE_SCOPE (which may or may not be a compound
// statement)
#define CREATE_SCOPE(Type,Var,...) \
    for (   Type Var __VA_ARGS__, *Sentinel_ = (void*)&Var; \
            Sentinel_; \
            Sentinel_ = NULL) \
//

#define for_counted_get(Tag,Name,List,Count) \
    if (!(Count)) { /* avoid calloc when Count is zero */ } else \
    CREATE_SCOPE(struct Tag*,Name,=calloc((size_t)Count,sizeof *Name),**Prev_ = &List) \
    for (SWord i_ = Count; i_ > 0; *Prev_ = Name, Prev_ = &Name++->next, i_--) \
//

static int get_recs(struct obj *o, STREAM *in, void *context)
{
    long *filesize = context;

    GET(o->rec_count, in);
    if (o->rec_count < 0) {
        errno = EINVAL;
        return 1;
    }
    if (o->rec_count > OBJ_MAX_REC_CNT) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.records = 1;
    for_counted_get(objrec, rec, o->records, o->rec_count) {
        GET(rec->addr, in);
        GET(rec->size, in);
        if (rec->size < 0) {
            errno = EINVAL;
            return 1;
        }
        long here = in->op.ftell(in);
        if (here < 0) {
            // not a seekable stream -- forge ahead recklessly
        } else if ((long)rec->size > *filesize - here) {
            errno = EFBIG;
            return 1;
        }
        rec->data = calloc((size_t)rec->size, sizeof *rec->data);
        assert(rec->data != NULL);
        get_sized(rec->data, sizeof *rec->data, (size_t)rec->size, in);
    }

    return 0;
}

static int get_syms_v2(struct obj *o, STREAM *in, void *context)
{
    GET(o->sym_count, in);
    if (o->sym_count < 0) {
        errno = EINVAL;
        return 1;
    }
    if (o->sym_count > OBJ_MAX_SYMBOLS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.symbols = 1;
    for_counted_get(objsym, sym, o->symbols, o->sym_count) {
        GET(sym->flags, in);
        GET(sym->name.len, in);
        if (sym->name.len > SYMBOL_LEN_V2) {
            errno = EFBIG;
            return 1;
        }
        size_t rounded_len = round_up_to_word((size_t)sym->name.len);
        sym->name.str = malloc(rounded_len + sizeof '\0');
        get_sized(sym->name.str, rounded_len, 1, in);
        sym->name.str[sym->name.len] = '\0';
        GET(sym->value, in);
        GET(sym->size, in);
    }

    return 0;
}

static int get_relocs_v2(struct obj *o, STREAM *in, void *context)
{
    GET(o->rlc_count, in);
    if (o->rlc_count < 0) {
        errno = EINVAL;
        return 1;
    }
    if (o->rlc_count > OBJ_MAX_RELOCS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.relocs = 1;
    for_counted_get(objrlc, rlc, o->relocs, o->rlc_count) {
        GET(rlc->flags, in);
        GET(rlc->name.len, in);
        if (rlc->name.len > SYMBOL_LEN_V2) {
            errno = EFBIG;
            return 1;
        }
        size_t rounded_len = round_up_to_word((size_t)rlc->name.len);
        rlc->name.str = malloc(rounded_len + sizeof '\0');
        get_sized(rlc->name.str, rounded_len, 1, in);
        rlc->name.str[rlc->name.len] = '\0';
        GET(rlc->addr, in);
        GET(rlc->width, in);
        GET(rlc->shift, in);
    }

    return 0;
}

static int obj_v2_read(struct obj *o, STREAM *in, const struct objops *ops)
{
    long where = in->op.ftell(in);
    long filesize = LONG_MAX;
    if (where >= 0 && !in->op.fseek(in, 0L, SEEK_END)) { // seekable stream
        filesize = in->op.ftell(in);
        if (in->op.fseek(in, where, SEEK_SET))
            fatal(PRINT_ERRNO, "Failed to seek input stream");
    }

    GET(o->flags, in);

    {
        int rc = 0;
        rc = ops->get_recs  (o, in, &filesize); if (rc) return rc;
        rc = ops->get_syms  (o, in,      NULL); if (rc) return rc;
        rc = ops->get_relocs(o, in,      NULL); if (rc) return rc;
    }

    return 0;
}

int obj_read(struct obj *o, STREAM *in)
{
    GET(o->magic.parsed.TOV, in);

    if (memcmp(o->magic.parsed.TOV, MAGIC_BYTES, sizeof o->magic.parsed.TOV))
        fatal(0, "Bad magic when loading object");

    GET(o->magic.parsed.version, in);

    int version = o->magic.parsed.version;
    switch (version) {
        case 2:
            return obj_v2_read(o, in, &objops);
        default:
            fatal(0, "Unhandled version number when loading object");
    }
}

static void obj_v2_free(struct obj *o)
{
#define obj_v2_free_helper(Tag, Field, Action)  \
    do {                                        \
        list_foreach(Tag, item, o->Field) {     \
            Action;                             \
            if (!o->bloc.Field)                 \
                free(item);                     \
        }                                       \
        if (o->bloc.Field)                      \
            free(o->Field);                     \
    } while (0)                                 \
    // end obj_v2_free_helper

    obj_v2_free_helper(objrlc, relocs , free(item->name.str));
    obj_v2_free_helper(objsym, symbols, free(item->name.str));
    obj_v2_free_helper(objrec, records, free(item->data    ));

    free(o);
}

void obj_free(struct obj *o)
{
    switch (o->magic.parsed.version) {
        case 2:
            obj_v2_free(o); break;
        default:
            fatal(0, "Unknown version number or corrupt memory while freeing object");
    }
}

/* vi: set ts=4 sw=4 et: */
