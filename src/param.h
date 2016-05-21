#ifndef PARAM_H_
#define PARAM_H_

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct param_state;

int param_get_int(struct param_state *pstate, const char *key, int *val);
/// @c param_get() returns true if key is found, false otherwise
int param_get(struct param_state *pstate, const char *key, size_t count, const void **val);
int param_set(struct param_state *pstate, char *key, void *val, int replace, int free_key, int free_value);
int param_add(struct param_state *pstate, const char *optarg);
void param_init(struct param_state **pstate);
void param_destroy(struct param_state *pstate);

#ifdef __cplusplus
};
#endif

#endif

/* vi: set ts=4 sw=4 et: */
