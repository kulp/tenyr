#define _XOPEN_SOURCE 600
#include "os_common.h"

#include "param.h"

#include <search.h>
#include <stdlib.h>
#include <string.h>

struct param_state {
#define DEFAULT_PARAMS_COUNT 16
    lfind_size_t params_count;
    size_t params_size;
    struct param_entry {
        char *key;
        unsigned free_key:1;
        struct string_list {
            struct string_list *next;
            unsigned free_value:1; ///< whether value should be free()d
            void *value;
        } *list;
    } *params;
};

static void param_free(struct param_entry *p);

static int params_cmp(const void *_a, const void *_b)
{
    const struct param_entry *a = _a,
                             *b = _b;

    return strcmp(a->key, b->key);
}

int param_get_int(struct param_state *pstate, const char *key, int *val)
{
    const char *str = NULL;
    char *next = NULL;
    int test = 0;
    if (param_get(pstate, key, 1, (const void **)&str) && str != NULL)
        test = strtol(str, &next, 0);

    int good = next > str;
    if (good)
        *val = test;

    return good;
}

int param_get(struct param_state *pstate, const char *key, size_t count, const void *val[count])
{
    if (!pstate) return 0; // permit calling with NULL param set

    const struct param_entry p = { .key = (char*)key };

    struct param_entry *q = lfind(&p, pstate->params, &pstate->params_count,
                                        sizeof *pstate->params, params_cmp);

    if (!q)
        return 0;

    struct string_list *r = q->list;
    size_t i = 0;
    for (; r; i++) {
        if (i < count)
            val[i] = r->value;
        r = r->next;
    }

    return i;
}

int param_set(struct param_state *pstate, char *key, void *val, int replace, int free_key, int free_value)
{
    if (!pstate) return 0; // permit calling with NULL param set

    if (pstate->params_size <= pstate->params_count) {
        while (pstate->params_size <= pstate->params_count)
            pstate->params_size *= 2;
        // technically there is a problem here if realloc() fails
        pstate->params = realloc(pstate->params, pstate->params_size * sizeof *pstate->params);
    }

    struct param_entry p = { .key  = key, .free_key = free_key }; // doesn't have a list yet
    struct param_entry *q = lsearch(&p, pstate->params, &pstate->params_count,
                                        sizeof *pstate->params, params_cmp);

    if (!q)
        return 1; // errno will be set

    struct string_list *list = calloc(1, sizeof *list);
    list->next = NULL;
    list->value = val;
    list->free_value = free_value;

    if (replace) {
        if (q->key != p.key) {
            param_free(q);
            *q = p;
        }
        q->list = list;
    } else {
        if (!q->list) {
            q->list = list;
        } else {
            struct string_list *r = q->list;
            while (r->next)
                r = r->next;
            r->next = list;
        }
    }

    return 0;
}

int param_add(struct param_state *pstate, const char *optarg)
{
    if (!pstate) return 0; // permit calling with NULL param set

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

    int replace = eq[-1] != '+';
    if (!replace)
        eq[-1] = '\0';

    return param_set(pstate, dupped, ++eq, replace, 1, 0);
}

void param_init(struct param_state **pstate)
{
    struct param_state *p = *pstate = calloc(1, sizeof **pstate);
    p->params_size  = DEFAULT_PARAMS_COUNT;
    p->params_count = 0;
    p->params       = calloc(p->params_size, sizeof *p->params);
}

void param_destroy(struct param_state *pstate)
{
    while (pstate->params_count--)
        param_free(&pstate->params[pstate->params_count]);

    free(pstate->params);
    free(pstate);
}

static void param_free(struct param_entry *p)
{
    if (p->free_key)
        free(p->key);
    struct string_list *q = p->list;
    while (q) {
        if (q->free_value)
            free(q->value);
        struct string_list *r = q;
        q = q->next;
        free(r);
    }
}

/* vi: set ts=4 sw=4 et: */
