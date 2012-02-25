// TODO make an mmap()-based version of this for systems that support it

#include "obj.h"

#include <stdlib.h>
#include <string.h>

#define MAGIC_BYTES "TOV"

#define PUT(What,Where) put_sized(&(What), sizeof (What), Where)
#define GET(What,Where) get_sized(&(What), sizeof (What), Where)

#define for_counted_put(Tag,Name,List,Count) \
    for (int _dummy = 0; !_dummy && (Count) > 0; _dummy++) \
        list_foreach(Tag, Name, List)

static inline void get_sized(void *what, size_t size, FILE *where)
{
    if (fread(what, size, 1, where) != 1)
        fatal(PRINT_ERRNO, "Unknown error in %s while parsing object", __func__);
}

static inline void put_sized(void *what, size_t size, FILE *where)
{
    if (fwrite(what, size, 1, where) != 1)
        fatal(PRINT_ERRNO, "Unknown error in %s while emitting object", __func__);
}

static int obj_v0_write(struct obj *o, FILE *out)
{
    put_sized(&MAGIC_BYTES, 3, out);
    PUT(o->magic.parsed.version, out);
    PUT(o->length, out);
    PUT(o->flags, out);

    PUT(o->rec_count, out);
    for_counted_put(objrec, rec, o->records, o->rec_count) {
        PUT(rec->addr, out);
        PUT(rec->size, out);
        if (fwrite(rec->data, sizeof *rec->data, rec->size, out) != rec->size)
            fatal(PRINT_ERRNO, "Unknown error in %s while emitting object", __func__);
    }

    PUT(o->sym_count, out);
    for_counted_put(objsym, sym, o->symbols, o->sym_count) {
        PUT(sym->flags, out);
        PUT(sym->name, out);
        PUT(sym->value, out);
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
            return -1; // never reached, but keeps compiler happy
    }
}

#define for_counted_get(Tag,Name,List,Count) \
    for (struct Tag *_f = NULL, *_l = NULL, *Name = NULL; \
            ((Count) ? Name ? !!(Count) : !!(Name = List = calloc(Count, sizeof *Name)) : 0) && !_f; \
            _f++) \
        for (UWord _i = (Count); _i > 0; _l ? (void)(_l->next = Name) : (void)0, _l = Name++, _i--)

static int obj_v0_read(struct obj *o, size_t *size, FILE *in)
{
    GET(o->length, in);
    GET(o->flags, in);

    GET(o->rec_count, in);
    for_counted_get(objrec, rec, o->records, o->rec_count) {
        GET(rec->addr, in);
        GET(rec->size, in);
        rec->data = calloc(rec->size, sizeof *rec->data);
        if (fread(rec->data, sizeof *rec->data, rec->size, in) != rec->size)
            fatal(PRINT_ERRNO, "Unknown error occurred while parsing object");
    }

    GET(o->sym_count, in);
    for_counted_get(objsym, sym, o->symbols, o->sym_count) {
        GET(sym->flags, in);
        GET(sym->name, in);
        GET(sym->value, in);
    }

    GET(o->rlc_count, in);
    for_counted_get(objrlc, rlc, o->relocs, o->rlc_count) {
        GET(rlc->flags, in);
        GET(rlc->name, in);
        GET(rlc->addr, in);
        GET(rlc->width, in);
    }

    // TODO this isn't actually useful ; change its semantics or remove it
    *size = sizeof *o;

    return 0;
}

int obj_read(struct obj *o, size_t *size, FILE *in)
{
    GET(o->magic.parsed.TOV, in);

    if (memcmp(o->magic.parsed.TOV, MAGIC_BYTES, sizeof o->magic.parsed.TOV))
        fatal(0, "Bad magic when loading object");

    GET(o->magic.parsed.version, in);

    switch (o->magic.parsed.version) {
        case 0: return obj_v0_read(o, size, in);
        default:
            fatal(0, "Unhandled version number when loading object");
    }

    return -1; // never reached, but keeps compiler happy
}

static void obj_v0_free(struct obj *o)
{
    UWord remaining = o->rec_count;
    list_foreach(objrec, rec, o->records) {
        if (remaining-- <= 0) break;
        free(rec->data);
    }

    free(o->records);
    free(o->symbols);
    free(o->relocs);

    free(o);
}

void obj_free(struct obj *o)
{
    switch (o->magic.parsed.version) {
        case 0: obj_v0_free(o); break;
        default:
            fatal(0, "Unknown version number or corrupt memory while freeing object");
            return; // never reached, but keeps compiler happy
    }
}

