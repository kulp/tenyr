/**
 * @file
 * Provides a lightweight integer-keyed key-value store.
 *
 * The key must be an integral type (default @c int), but can be any integral
 * type if @c KV_KEY_TYPE is defined before @c #include'ing this file.
 */

#ifndef KV_INT_H_
#define KV_INT_H_

#include <stdlib.h>
#include <search.h>

#ifndef KV_KEY_TYPE
#define KV_KEY_TYPE int
#endif

#ifndef KV_VAL_TYPE
#define KV_VAL_TYPE  void*
#define KV_VAL_EMPTY NULL
#endif

struct _kv {
    KV_KEY_TYPE key;
    KV_VAL_TYPE val;
};

static int _kv_int_cmp(const void *a, const void *b)
{
    return ((struct _kv*)a)->key - ((struct _kv*)b)->key;
}

static inline struct _kv** _kv_int_fetch(void **store, KV_KEY_TYPE key)
{
    return tfind(&(struct _kv){ .key = key }, store, _kv_int_cmp);
}

static inline KV_VAL_TYPE *kv_int_get(void **store, KV_KEY_TYPE key)
{
    struct _kv **x = _kv_int_fetch(store, key);
    return x ? &x[0]->val : KV_VAL_EMPTY;
}

static inline KV_VAL_TYPE *kv_int_remove(void **store, KV_KEY_TYPE key)
{
    struct _kv **what = _kv_int_fetch(store, key);
    if (!what) return KV_VAL_EMPTY;
    struct _kv *tf = *what;
    tdelete(tf, store, _kv_int_cmp);
    KV_VAL_TYPE *result = &tf->val;
    free(tf);
    return result; // NOTE *result is not valid, but result is non-NULL
}

static inline KV_VAL_TYPE *kv_int_put(void **store, KV_KEY_TYPE key, KV_VAL_TYPE *val)
{
    kv_int_remove(store, key);
    struct _kv *what = malloc(sizeof *what);
    what->key = key;
    what->val = *val;
    tsearch(what, store, _kv_int_cmp);
    return &what->val;
}

static inline void kv_int_init(void **store)
{
    *store = NULL;
}

#endif /* KV_INT_H_ */

/* vim:set et ts=4 sw=4 syntax=c.doxygen: */

