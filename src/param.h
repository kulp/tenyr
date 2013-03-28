#ifndef PARAM_H_
#define PARAM_H_

#include <stddef.h>

struct param_state;

/// @c param_get() returns true if key is found, false otherwise
int param_get(struct param_state *pstate, char *key, size_t count, const char *val[count]);
int param_set(struct param_state *pstate, char *key, char *val, int replace, int free_value);
int param_add(struct param_state *pstate, const char *optarg);
void param_init(struct param_state **pstate);
void param_destroy(struct param_state *pstate);

#endif

