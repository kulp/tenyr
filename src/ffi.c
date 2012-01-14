#include "ffi.h"
#include "common.h"

#include <assert.h>
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

static int at_pc(struct mstate *m, void *cud)
{
    uint32_t *pc = cud;
    return m->regs[15] == *pc;
}

int tf_run_until(struct state *s, uint32_t start_address, int flags, cont_pred
        stop, void *cud)
{
    int rc = 0;

    while (!stop(&s->machine, cud)) {
        assert(("PC within address space", !(s->machine.regs[15] & ~PTR_MASK)));
        struct instruction i;
        s->dispatch_op(s, OP_READ, s->machine.regs[15], &i.u.word);

        if (run_instruction(s, &i))
            return 1;
    }

    return rc;
}

int tf_get_addr(const struct state *s, const char *symbol, uint32_t *addr)
{
    int rc = 0;

    *addr = 0; // XXX

    return rc;
}

int tf_call(struct state *s, const char *symbol)
{
    int rc = 0;
    uint32_t addr, nextaddr;
    rc = tf_get_addr(s, symbol, &addr);
    if (rc)
        return rc;

    nextaddr = addr + 1;
    // TODO need to create call shim so we don't just execute one instruction
    // and stop
    rc = tf_run_until(s, addr, 0, at_pc, &nextaddr);

    return rc;
}

