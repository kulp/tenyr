// TODO make an mmap()-based version of this for systems that support it

#include "obj.h"

#include <stdlib.h>
#include <string.h>

#define MAGIC_BYTES "TOV"

#define PUTSIZED(What,Size,Where) \
    do { if (fwrite(&(What), (Size), 1, (Where)) != 1) goto bad; } while (0)
#define PUT(What,Where) \
    PUTSIZED(What, sizeof (What), Where)

#define GETSIZED(What,Size,Where) \
    do { if (fread(&(What), (Size), 1, (Where)) != 1) goto bad; } while (0)
#define GET(What,Where) \
    GETSIZED(What, sizeof (What), Where)

static int obj_v0_write(struct obj_v0 *o, FILE *out)
{
    PUTSIZED(MAGIC_BYTES, 3, out);
    PUT(o->base.magic.parsed.version, out);
    PUT(o->length, out);
    PUT(o->flags, out);
    PUT(o->rec_count, out);

    {
        UWord remaining = o->rec_count;
        struct objrec *rec = o->records;
        while (rec && remaining-- > 0) {
            PUT(rec->addr, out);
            PUT(rec->size, out);
            if (fwrite(rec->data, sizeof *rec->data, rec->size, out) != rec->size)
                goto bad;

            rec = rec->next;
        }
    }

    {
        UWord remaining = o->sym_count;
        struct objsym *sym = o->symbols;
        while (sym && remaining-- > 0) {
            PUT(sym->flags, out);
            PUT(sym->name, out);
            PUT(sym->value, out);

            sym = sym->next;
        }
    }

    return 0;
bad:
    fatal("Unknown error occurred while emitting object", 0);
    return -1; // never reached, but keeps compiler happy
}

int obj_write(struct obj *o, FILE *out)
{
    switch (o->magic.parsed.version) {
        case 0: return obj_v0_write((void*)o, out);
        default:
            goto bad;
    }

bad:
    fatal("Unhandled version while emitting object", 0);
    return -1; // never reached, but keeps compiler happy
}

static int obj_v0_read(struct obj_v0 *o, size_t *size, FILE *in)
{
    GET(o->length, in);
    GET(o->flags, in);
    GET(o->rec_count, in);

    {
        UWord remaining = o->rec_count;
        if (remaining) {
            struct objrec *last = NULL,
                          *rec  = o->records = calloc(remaining, sizeof *rec);
            while (remaining-- > 0) {
                GET(rec->addr, in);
                GET(rec->size, in);
                rec->data = calloc(rec->size, sizeof *rec->data);
                if (fread(rec->data, sizeof *rec->data, rec->size, in) != rec->size)
                    goto bad;

                rec->prev = last;
                if (last) last->next = rec;
                last = rec;
                rec++;
            }
        }
    }

    {
        UWord remaining = o->sym_count;
        if (remaining) {
            struct objsym *last = NULL,
                          *sym  = o->symbols = calloc(remaining, sizeof *sym);
            while (remaining-- > 0) {
                GET(sym->flags, in);
                GET(sym->name, in);
                GET(sym->value, in);

                sym->prev = last;
                if (last) last->next = sym;
                last = sym;
                sym++;
            }
        }
    }

    {
        UWord remaining = o->rlc_count;
        if (remaining) {
            struct objrlc *last = NULL,
                          *rlc  = o->relocs = calloc(remaining, sizeof *rlc);
            while (remaining-- > 0) {
                GET(rlc->flags, in);
                GET(rlc->name, in);
                GET(rlc->addr, in);
                GET(rlc->width, in);

                rlc->prev = last;
                if (last) last->next = rlc;
                last = rlc;
                rlc++;
            }
        }
    }

    // TODO this isn't actually useful ; change its semantics or remove it
    *size = sizeof *o;

    return 0;
bad:
    fatal("Unknown error occurred while parsing object", 0);
    return -1; // never reached, but keeps compiler happy
}

int obj_read(struct obj *o, size_t *size, FILE *in)
{
    char buf[3];
    GET(buf, in);

    if (memcmp(buf, MAGIC_BYTES, sizeof buf))
        goto bad;

    GET(o->magic.parsed.version, in);

    switch (o->magic.parsed.version) {
        case 0: return obj_v0_read((void*)o, size, in);
        default:
            fatal("Unhandled version number when loading object", 0);
    }
bad:
    fatal("Bad magic when loading object", 0);
    return -1; // never reached, but keeps compiler happy
}

static void obj_v0_free(struct obj_v0 *o)
{
    UWord remaining = o->rec_count;
    struct objrec *rec = o->records;
    while (rec && remaining-- > 0) {
        struct objrec *temp = rec->next;
        free(rec->data);
        free(rec);
        rec = temp;
    }

    free(o);
}

void obj_free(struct obj *o)
{
    switch (o->magic.parsed.version) {
        case 0: obj_v0_free((void*)o); break;
        default:
            goto bad;
    }

    return;
bad:
    fatal("Unknown error occurred while freeing object", 0);
    return; // never reached, but keeps compiler happy
}

