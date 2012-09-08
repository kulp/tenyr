#define _XOPEN_SOURCE 600

#include "param.h"

#include <search.h>
#include <stdlib.h>
#include <string.h>

static void param_free(struct param_entry *p);

static int params_cmp(const void *_a, const void *_b)
{
    const struct param_entry *a = _a,
                             *b = _b;

    return strcmp(a->key, b->key);
}

int param_get(struct param_state *pstate, char *key, const char **val)
{
    struct param_entry p = { .key = key };

    struct param_entry *q = lfind(&p, pstate->params, &pstate->params_count,
                                        sizeof *pstate->params, params_cmp);

    if (!q)
        return 0;

    *val = q->value;

    return 1;
}

int param_set(struct param_state *pstate, char *key, char *val, int free_value)
{
    while (pstate->params_size <= pstate->params_count)
        // technically there is a problem here if realloc() fails
        pstate->params = realloc(pstate->params,
                (pstate->params_size *= 2) * sizeof *pstate->params);

    struct param_entry p = {
        .key        = key,
        .value      = val,
        .free_value = free_value,
    };

    struct param_entry *q = lsearch(&p, pstate->params, &pstate->params_count,
                                        sizeof *pstate->params, params_cmp);

    if (!q)
        return 1;

    if (q->key != p.key) {
        param_free(q);
        *q = p;
    }

    return 0;
}

int param_add(struct param_state *pstate, const char *optarg)
{
    // We can't use getsubopt() here because we don't know what all of our
    // options are ahead of time.
    char *dupped = strdup(optarg);
    char *eq = strchr(dupped, '=');
    if (!eq) {
        free(dupped);
        return 1;
    }

    // Replace '=' with '\0' to split string in two
    *eq = '\0';

    return param_set(pstate, dupped, ++eq, 0);
}

void param_destroy(struct param_state *pstate)
{
    while (pstate->params_count--)
        param_free(&pstate->params[pstate->params_count]);

    free(pstate->params);
    pstate->params_size = 0;
}

static void param_free(struct param_entry *p)
{
    free(p->key);
    if (p->free_value)
        free(p->value);
}

