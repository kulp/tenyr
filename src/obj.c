// TODO make an mmap()-based version of this for systems that support it

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

#define for_counted_put(Tag,Name,List,Count) \
    for (int _dummy = 0; !_dummy && (Count) > 0; _dummy++) \
        list_foreach(Tag, Name, List)

static inline void get_sized_le(void *what, size_t size, size_t count, FILE *where)
{
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

static int obj_v0_write(struct obj *o, FILE *out)
{
    put_sized(MAGIC_BYTES, 3, 1, out);
    PUT(o->magic.parsed.version, out);
    PUT(o->flags, out);

    PUT(o->rec_count, out);
    for_counted_put(objrec, rec, o->records, o->rec_count) {
        PUT(rec->addr, out);
        PUT(rec->size, out);
        put_sized(rec->data, sizeof *rec->data, rec->size, out);
    }

    PUT(o->sym_count, out);
    for_counted_put(objsym, sym, o->symbols, o->sym_count) {
        PUT(sym->flags, out);
        PUT(sym->name, out);
        PUT(sym->value, out);
        PUT(sym->size, out);
    }

    PUT(o->rlc_count, out);
    for_counted_put(objrlc, rlc, o->relocs, o->rlc_count) {
        PUT(rlc->flags, out);
        PUT(rlc->name, out);
        PUT(rlc->addr, out);
        PUT(rlc->width, out);
    }

    return 0;
}

int obj_write(struct obj *o, FILE *out)
{
    switch (o->magic.parsed.version) {
        case 0: return obj_v0_write(o, out);
        default:
            fatal(0, "Unhandled version while emitting object");
    }
}

#define for_counted_get(Tag,Name,List,Count) \
    if (((List) = NULL)) {} else for (struct Tag *_f = NULL, *_l = NULL, *Name = NULL; \
            ((Count) ? Name ? !!(Count) : (!!(Name = List = calloc(Count, sizeof *Name)) || \
                (fatal(PRINT_ERRNO, "Failed to allocate %d records for %s", Count, #List),0)) : 0) && !_f; \
            _f++) \
        for (UWord _i = (Count); _i > 0; _l ? (void)(_l->next = Name) : (void)0, _l = Name++, _i--)

static int obj_v0_read(struct obj *o, FILE *in)
{
    long where = ftell(in);
    long filesize = LONG_MAX;
    if (!fseek(in, 0L, SEEK_END)) { // seekable stream
        filesize = ftell(in);
        if (where < 0 || fseek(in, where, SEEK_SET))
            fatal(PRINT_ERRNO, "Failed to seek input stream");
    }

    GET(o->flags, in);

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
        if (rec->size + here > (unsigned)filesize) {
            errno = EFBIG;
            return 1;
        }
        rec->data = calloc(rec->size, sizeof *rec->data);
        if (!rec->data)
            fatal(PRINT_ERRNO, "Failed to allocate record data field");
        get_sized(rec->data, sizeof *rec->data, rec->size, in);
    }

    GET(o->sym_count, in);
    if (o->sym_count > OBJ_MAX_SYMBOLS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.symbols = 1;
    for_counted_get(objsym, sym, o->symbols, o->sym_count) {
        GET(sym->flags, in);
        GET(sym->name, in);
        GET(sym->value, in);
        GET(sym->size, in);
    }

    GET(o->rlc_count, in);
    if (o->rlc_count > OBJ_MAX_RELOCS) {
        errno = EFBIG;
        return 1;
    }
    o->bloc.relocs = 1;
    for_counted_get(objrlc, rlc, o->relocs, o->rlc_count) {
        GET(rlc->flags, in);
        GET(rlc->name, in);
        GET(rlc->addr, in);
        GET(rlc->width, in);
    }

    return 0;
}

int obj_read(struct obj *o, FILE *in)
{
    GET(o->magic.parsed.TOV, in);

    if (memcmp(o->magic.parsed.TOV, MAGIC_BYTES, sizeof o->magic.parsed.TOV))
        fatal(0, "Bad magic when loading object");

    GET(o->magic.parsed.version, in);

    switch (o->magic.parsed.version) {
        case 0: return obj_v0_read(o, in);
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

void obj_free(struct obj *o)
{
    switch (o->magic.parsed.version) {
        case 0: obj_v0_free(o); break;
        default:
            fatal(0, "Unknown version number or corrupt memory while freeing object");
    }
}

/* vi: set ts=4 sw=4 et: */
