// TODO make an mmap()-based version of this for systems that support it

// `context` parameters not often used in obj_op
#pragma GCC diagnostic ignored "-Wunused-parameter"

#include "obj.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#define MAGIC_BYTES "TOV"
#define OBJ_MAX_SYMBOLS  ((1 << 16) - 1)    /* arbitrary safety limit */
#define OBJ_MAX_RELOCS   ((1 << 16) - 1)    /* arbitrary safety limit */
#define OBJ_MAX_REC_SIZE ((1ULL << 31) - 1) /* maximum meaningful size */
#define OBJ_MAX_REC_CNT  ((1 << 16) - 1)    /* arbitrary safety limit */

#define PUT(What,Where) put_sized(&(What), sizeof (What), 1, Where)
#define GET(What,Where) get_sized(&(What), sizeof (What), 1, Where)

typedef int obj_op(struct obj *o, FILE *out, void *context);

static obj_op
    put_recs     , get_recs     ,
    put_syms_v0  , get_syms_v0  ,
    put_syms_v1  , get_syms_v1  ,
    put_syms_v2  , get_syms_v2  ,
    put_relocs_v0, get_relocs_v0,
    put_relocs_v1, get_relocs_v1,
    put_relocs_v2, get_relocs_v2;

static struct objops {
    obj_op *put_recs, *put_syms, *put_relocs,
           *get_recs, *get_syms, *get_relocs;
} objops[] = {
    [0] = {
        .put_recs = put_recs, .put_syms = put_syms_v0, .put_relocs = put_relocs_v0,
        .get_recs = get_recs, .get_syms = get_syms_v0, .get_relocs = get_relocs_v0,
    },
    [1] = {
        .put_recs = put_recs, .put_syms = put_syms_v1, .put_relocs = put_relocs_v1,
        .get_recs = get_recs, .get_syms = get_syms_v1, .get_relocs = get_relocs_v1,
    },
    [2] = {
        .put_recs = put_recs, .put_syms = put_syms_v2, .put_relocs = put_relocs_v2,
        .get_recs = get_recs, .get_syms = get_syms_v2, .get_relocs = get_relocs_v2,
    },
};

static inline void get_sized_le(void *what, size_t size, size_t count, FILE *where)
{
    if (size * count == 0) {
        return;
    }
    if (fread(what, size, count, where) != count) {
        if (feof(where)) {
            fatal(0, "End of file unexpectedly reached while parsing object");
        } else {
            fatal(0, "Unknown error while parsing object");
        }
    }
}

static inline void put_sized_le(const void *what, size_t size, size_t count, FILE *where)
{
    if (size * count == 0) {
        return;
    }
    if (fwrite(what, size, count, where) != count) {
        if (feof(where)) {
            fatal(0, "End of file unexpectedly reached while emitting object");
        } else {
            fatal(0, "Unknown error while emitting object");
        }
    }
}

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define get_sized get_sized_le
#define put_sized put_sized_le
#else
static inline void get_sized_be(void *what, size_t size, size_t count, FILE *where)
{
    get_sized_le(what, size, count, where);
    // get_sized() isn't as general as it looks - it does bytes, UWords, and
    // strings.
    if (size == sizeof(UWord)) {
        UWord *dest = what;
        for (size_t i = 0; i < count; i++)
            dest[i] = swapword(dest[i]);
    }
}

static inline void put_sized_be(const void *what, size_t size, size_t count, FILE *where)
{
    // put_sized() has an analagous caveat to get_sized()'s, but we only swap
    // one word at a time so we don't have to allocate arbitrarily-large
    // buffers.
    if (size == sizeof(UWord)) {
        const UWord *src = what;
        for (size_t i = 0; i < count; i++) {
            const UWord temp = swapword(src[i]);
            put_sized_le(&temp, sizeof(UWord), 1, where);
        }
    } else {
        put_sized_le(what, size, count, where);
    }
}

#define get_sized get_sized_be
#define put_sized put_sized_be
#endif

static int put_recs(struct obj *o, FILE *out, void *context)
{
    PUT(o->rec_count, out);
    list_foreach(objrec, rec, o->records) {
        PUT(rec->addr, out);
        PUT(rec->size, out);
        put_sized(rec->data, sizeof *rec->data, rec->size, out);
    }

    return 0;
}

static int put_syms_v0(struct obj *o, FILE *out, void *context)
{
    return put_syms_v1(o, out, context);
}

static int put_syms_v1(struct obj *o, FILE *out, void *context)
{
    PUT(o->sym_count, out);
    list_foreach(objsym, sym, o->symbols) {
        PUT(sym->flags, out);
        char buf[SYMBOL_LEN_V1] = { 0 };
        if (sym->name.str) strncpy(buf, sym->name.str, sizeof buf);
        PUT(buf, out);
        PUT(sym->value, out);
        PUT(sym->size, out);
    }

    return 0;
}

static int put_syms_v2(struct obj *o, FILE *out, void *context)
{
    PUT(o->sym_count, out);
    list_foreach(objsym, sym, o->symbols) {
        PUT(sym->flags, out);
        PUT(sym->name.len, out);
        put_sized(sym->name.str, round_up_to_word(sym->name.len), 1, out);
        PUT(sym->value, out);
        PUT(sym->size, out);
    }

    return 0;
}

static int put_relocs_v0(struct obj *o, FILE *out, void *context)
{
    PUT(o->rlc_count, out);
    list_foreach(objrlc, rlc, o->relocs) {
        PUT(rlc->flags, out);
        char buf[SYMBOL_LEN_V1] = { 0 };
        if (rlc->name.str) strncpy(buf, rlc->name.str, sizeof buf);
        PUT(buf, out);
        PUT(rlc->addr, out);
        PUT(rlc->width, out);
    }

    return 0;
}

static int put_relocs_v1(struct obj *o, FILE *out, void *context)
{
    PUT(o->rlc_count, out);
    list_foreach(objrlc, rlc, o->relocs) {
        PUT(rlc->flags, out);
        char buf[SYMBOL_LEN_V1] = { 0 };
        if (rlc->name.str) strncpy(buf, rlc->name.str, sizeof buf);
        PUT(buf, out);
        PUT(rlc->addr, out);
        PUT(rlc->width, out);
        PUT(rlc->shift, out);
    }

    return 0;
}

static int put_relocs_v2(struct obj *o, FILE *out, void *context)
{
    PUT(o->rlc_count, out);
    list_foreach(objrlc, rlc, o->relocs) {
        PUT(rlc->flags, out);
        PUT(rlc->name.len, out);
        put_sized(rlc->name.str, round_up_to_word(rlc->name.len), 1, out);
        PUT(rlc->addr, out);
        PUT(rlc->width, out);
        PUT(rlc->shift, out);
    }

    return 0;
}

static int obj_vx_write(struct obj *o, FILE *out, struct objops *ops)
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

int obj_write(struct obj *o, FILE *out)
{
    int version = o->magic.parsed.version;

    switch (version) {
        case 0: case 1: case 2:
            return obj_vx_write(o, out, &objops[version]);
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
    CREATE_SCOPE(struct Tag*,Name,=calloc(Count,sizeof *Name),**Prev_ = &List) \
    for (UWord i_ = Count; i_ > 0; *Prev_ = Name, Prev_ = &Name++->next, i_--) \
//

static int get_recs(struct obj *o, FILE *in, void *context)
{
    int *filesize = context;

    GET(o->rec_count, in);
    if (o->rec_count > OBJ_MAX_REC_CNT) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.records = 1;
    for_counted_get(objrec, rec, o->records, o->rec_count) {
        GET(rec->addr, in);
        GET(rec->size, in);
        if (rec->size > OBJ_MAX_REC_SIZE) {
            errno = EFBIG;
            return 1;
        }
        long here = ftell(in);
        if (here < 0) {
            // not a seekable stream -- forge ahead recklessly
        } else if (rec->size + here > (unsigned)*filesize) {
            errno = EFBIG;
            return 1;
        }
        rec->data = calloc(rec->size, sizeof *rec->data);
        if (!rec->data)
            fatal(PRINT_ERRNO, "Failed to allocate record data field");
        get_sized(rec->data, sizeof *rec->data, rec->size, in);
    }

    return 0;
}

static int get_syms_v0(struct obj *o, FILE *in, void *context)
{
    return get_syms_v1(o, in, context);
}

static int get_syms_v1(struct obj *o, FILE *in, void *context)
{
    GET(o->sym_count, in);
    if (o->sym_count > OBJ_MAX_SYMBOLS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.symbols = 1;
    for_counted_get(objsym, sym, o->symbols, o->sym_count) {
        GET(sym->flags, in);
        sym->name.str = malloc(SYMBOL_LEN_V1 + sizeof '\0');
        get_sized(sym->name.str, SYMBOL_LEN_V1, 1, in);
        sym->name.str[SYMBOL_LEN_V1] = '\0';
        sym->name.len = strlen(sym->name.str);
        GET(sym->value, in);
        GET(sym->size, in);
    }

    return 0;
}

static int get_syms_v2(struct obj *o, FILE *in, void *context)
{
    GET(o->sym_count, in);
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
        size_t rounded_len = round_up_to_word(sym->name.len);
        sym->name.str = malloc(rounded_len + sizeof '\0');
        get_sized(sym->name.str, rounded_len, 1, in);
        sym->name.str[sym->name.len] = '\0';
        GET(sym->value, in);
        GET(sym->size, in);
    }

    return 0;
}

static int get_relocs_v0(struct obj *o, FILE *in, void *context)
{
    GET(o->rlc_count, in);
    if (o->rlc_count > OBJ_MAX_RELOCS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.relocs = 1;
    for_counted_get(objrlc, rlc, o->relocs, o->rlc_count) {
        GET(rlc->flags, in);
        rlc->name.str = malloc(SYMBOL_LEN_V1 + sizeof '\0');
        get_sized(rlc->name.str, SYMBOL_LEN_V1, 1, in);
        rlc->name.str[SYMBOL_LEN_V1] = '\0';
        rlc->name.len = strlen(rlc->name.str);
        GET(rlc->addr, in);
        GET(rlc->width, in);
        rlc->shift = 0;
    }

    return 0;
}

static int get_relocs_v2(struct obj *o, FILE *in, void *context)
{
    GET(o->rlc_count, in);
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
        size_t rounded_len = round_up_to_word(rlc->name.len);
        rlc->name.str = malloc(rounded_len + sizeof '\0');
        get_sized(rlc->name.str, rounded_len, 1, in);
        rlc->name.str[rlc->name.len] = '\0';
        GET(rlc->addr, in);
        GET(rlc->width, in);
        GET(rlc->shift, in);
    }

    return 0;
}

static int get_relocs_v1(struct obj *o, FILE *in, void *context)
{
    GET(o->rlc_count, in);
    if (o->rlc_count > OBJ_MAX_RELOCS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.relocs = 1;
    for_counted_get(objrlc, rlc, o->relocs, o->rlc_count) {
        GET(rlc->flags, in);
        rlc->name.str = malloc(SYMBOL_LEN_V1 + sizeof '\0');
        get_sized(rlc->name.str, SYMBOL_LEN_V1, 1, in);
        rlc->name.str[SYMBOL_LEN_V1] = '\0';
        rlc->name.len = strlen(rlc->name.str);
        GET(rlc->addr, in);
        GET(rlc->width, in);
        GET(rlc->shift, in);
    }

    return 0;
}

static int obj_vx_read(struct obj *o, FILE *in, struct objops *ops)
{
    long where = ftell(in);
    long filesize = LONG_MAX;
    if (where >= 0 && !fseek(in, 0L, SEEK_END)) { // seekable stream
        filesize = ftell(in);
        if (fseek(in, where, SEEK_SET))
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

int obj_read(struct obj *o, FILE *in)
{
    GET(o->magic.parsed.TOV, in);

    if (memcmp(o->magic.parsed.TOV, MAGIC_BYTES, sizeof o->magic.parsed.TOV))
        fatal(0, "Bad magic when loading object");

    GET(o->magic.parsed.version, in);

    int version = o->magic.parsed.version;
    switch (version) {
        case 0: case 1: case 2:
            return obj_vx_read(o, in, &objops[version]);
        default:
            fatal(0, "Unhandled version number when loading object");
    }
}

static void obj_v0_free(struct obj *o)
{
    UWord remaining = o->rec_count;
    list_foreach(objrec, rec, o->records) {
        if (remaining-- <= 0) break;
        free(rec->data);
    }

    if (o->bloc.relocs) free(o->relocs);
    else list_foreach(objrlc,rlc,o->relocs) free(rlc);

    if (o->bloc.symbols) free(o->symbols);
    else list_foreach(objsym,sym,o->symbols) free(sym);

    if (o->bloc.records) free(o->records);
    else list_foreach(objrec,rec,o->records) free(rec);

    free(o);
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
        case 0: case 1:
            obj_v0_free(o); break;
        case 2:
            obj_v2_free(o); break;
        default:
            fatal(0, "Unknown version number or corrupt memory while freeing object");
    }
}

/* vi: set ts=4 sw=4 et: */
