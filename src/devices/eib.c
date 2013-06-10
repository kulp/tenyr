#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <sys/time.h>

#include "plugin.h"

#include "common.h"
#include "device.h"
#include "sim.h"

// TODO include EIB macros more generically
#include "../lib/irq.th"

#define MAX_DEPTH   32
#define STACK_BITS  5
#define STACK_SIZE  (1 << STACK_BITS)
#define VEC_SIZE    32
#define TRAMP_BITS  8
#define TRAMP_SIZE  (1 << TRAMP_BITS)

struct eib_state {
    int depth;
    struct eib_stack {
        uint32_t istack[STACK_SIZE];
        uint32_t imr;
        uint32_t ret;
    } stack[MAX_DEPTH];
    uint32_t vecs[VEC_SIZE];
    uint32_t tramp[TRAMP_SIZE];
    uint32_t isr;
};

#define PARAM_GET(Cookie,Key,Count,Val) \
    (Cookie)->gops.param_get(Cookie, Key, Count, Val)
#define PARAM_SET(Cookie,Key,Val,Replace,ShouldFree) \
    (Cookie)->gops.param_set(Cookie, Key, Val, Replace, ShouldFree)

// This is not intended to be a fully robust $readmemh() replacement.
static int readmemh(FILE *fp, size_t len, uint32_t array[len])
{
    uint32_t addr = 0;
    char buf[BUFSIZ], *end = &buf[BUFSIZ];
    while (!feof(fp) && addr < len) {
        // really large hex numbers could break this
        if (!fgets(buf, sizeof buf, fp))
            break;

        char *p = buf;

        while (p < end && *p && isspace(*p)) p++;

        while (p < end && *p && *p == '@') {
            addr = numberise(++p, 16);
            while (p < end && *p && isxdigit(*p)) p++;
            while (p < end && *p && isspace(*p) ) p++;
        }

        if (p < end && *p)
            array[addr++] = numberise(p, 16);
    }

    return 0;
}

static int load_tramp(struct eib_state *state, struct plugin_cookie *pcookie)
{
    int rc = 0;

    char *filename;
    PARAM_GET(pcookie, "eib.trampoline", 1, &filename);
    if (!filename)
        return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp)
        rc = readmemh(fp, countof(state->tramp), state->tramp);
    if (!fp || rc)
        fatal(PRINT_ERRNO, "Could not load trampoline from `%s'", filename);

    return rc;
}

static int load_vecs(struct eib_state *state, struct plugin_cookie *pcookie)
{
    int rc = 0;

    char *filename;
    PARAM_GET(pcookie, "eib.vectors", 1, &filename);
    if (!filename)
        return 0;

    FILE *fp = fopen(filename, "rb");
    if (fp)
        rc = readmemh(fp, countof(state->vecs), state->vecs);
    if (!fp || rc)
        fatal(PRINT_ERRNO, "Could not load vectors from `%s'", filename);

    return rc;
}

static int eib_init(struct plugin_cookie *pcookie, void *cookie, int nargs, ...)
{
    int rc = 0;

    struct eib_state *state = *(void**)cookie;

    if (!state)
        state = *(void**)cookie = malloc(sizeof *state);

    *state = (struct eib_state){ .depth = 0 };

    load_vecs(state, pcookie);
    load_tramp(state, pcookie);

    return rc;
}

static int eib_fini(void *cookie)
{
    struct eib_state *state = cookie;

    free(state);

    return 0;
}

static int eib_op(void *cookie, int op, uint32_t addr, uint32_t *data)
{
    struct eib_state *state = cookie;

    return 0;
}

static int eib_pump(void *cookie)
{
    struct eib_state *state = cookie;

    return 0;
}

#if PLUGIN
void EXPORT tenyr_plugin_init(struct guest_ops *ops)
{
    fatal_ = ops->fatal;
    debug_ = ops->debug;
}
#endif

int EXPORT eib_add_device(struct device **device)
{
    **device = (struct device){
        .bounds = { EIB_BOTTOM, EIB_TOP },
        .ops = {
            .op = eib_op,
            .init = eib_init,
            .fini = eib_fini,
            .cycle = eib_pump,
        },
    };

    return 0;
}

