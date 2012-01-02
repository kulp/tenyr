// TODO make an mmap()-based version of this for systems that support it

#include "obj.h"

#include <stdlib.h>
#include <string.h>

#define MAGIC_BYTES         "TOV"
#define SUPPORTED_VERSION   0

int obj_write(struct obj *_o, FILE *out)
{
    switch (_o->magic.parsed.version) {
        case 0: {
            struct obj_v0 *o = (void*)_o;
        #define PUTSIZED(What,Size,Where) \
            do { if (fwrite(&(What), (Size), 1, (Where)) != 1) goto bad; } while (0)
        #define PUT(What,Where) \
            PUTSIZED(What, sizeof (What), Where)

            PUT(MAGIC_BYTES, out);
            PUT(_o->magic.parsed.version, out);
            PUT(o->length, out);
            PUT(o->flags, out);
            PUT(o->count, out);

            UWord remaining = o->count;
            struct objrec *rec = o->records;
            while (rec && remaining-- > 0) {
                PUT(rec->addr, out);
                PUT(rec->size, out);
                if (fwrite(rec->data, rec->size, 1, out) != 1)
                    goto bad;

                rec = rec->next;
            }

            return 0;
        }
        default:
            goto bad;
    }

bad:
    abort(); // XXX better error reporting
}

int obj_read(struct obj *_o, FILE *in)
{
#define GETSIZED(What,Size,Where) \
    do { if (fread(&(What), (Size), 1, (Where)) != 1) goto bad; } while (0)
#define GET(What,Where) \
    GETSIZED(What, sizeof (What), Where)

    struct obj_v0 *o = (void*)_o;

    char buf[3];
    GET(buf, in);

    if (memcmp(buf, MAGIC_BYTES, sizeof buf))
        goto bad;

    GET(_o->magic.parsed.version, in);

    if (_o->magic.parsed.version > SUPPORTED_VERSION)
        goto bad;

    GET(o->length, in);
    GET(o->flags, in);
    GET(o->count, in);

    UWord remaining = o->count;
    struct objrec *last = NULL,
                  *rec  = o->records = calloc(remaining, sizeof *rec);
    while (remaining-- > 0) {
        GET(rec->addr, in);
        GET(rec->size, in);
        rec->data = calloc(rec->size, 1);
        if (fread(rec->data, rec->size, 1, in) != 1)
            goto bad;

        rec->prev = last;
        if (last) last->next = rec;
        last = rec;
        rec++;
    }

    last->next = rec;

    return 0;
bad:
    abort(); // XXX better error reporting
}

