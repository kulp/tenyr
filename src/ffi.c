#include "ffi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#endif

// TODO this will need to be upgraded to use real objects when those are
// implemented (probably will just wrap a call to the obj API)
int tf_read_file(struct obj *o, const char *filename)
{
    int rc = 0;

    FILE *f = fopen(filename, "r");
    if (!f)
        return -1;

    while (!feof(f)) {
        char buf[BUFSIZ];
        size_t result = fread(buf, 1, sizeof buf, f);
        if (result == 0)
            goto bad;

        size_t nextsize = o->used + result;
        if (nextsize > o->allocated) {
            if (o->allocated == 0) {
                o->data = malloc(o->allocated = MAX(BUFSIZ, nextsize));
            } else {
                while (nextsize > o->allocated)
                    o->allocated *= 2;
                // realloc (or malloc) could fail ; trap ?
                o->data = realloc(o->data, o->allocated);
            }
        }

        memcpy(&o->data[o->used], buf, result);
        o->used = nextsize;

        if (result < sizeof buf)
            break;
    }

done:
    fclose(f);
    return rc;
bad:
    rc = -1;
    goto done;
}

