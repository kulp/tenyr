#ifndef PARAM_H_
#define PARAM_H_

#include <stddef.h>

struct param_state {
#define DEFAULT_PARAMS_COUNT 16
    size_t params_count;
    size_t params_size;
    struct param_entry {
        char *key;
        char *value;
        int free_value; ///< whether value should be free()d
    } *params;
};

/// @c param_get() returns true if key is found, false otherwise
int param_get(struct param_state *pstate, char *key, const char **val);
int param_set(struct param_state *pstate, char *key, char *val, int free_value);
int param_add(struct param_state *pstate, const char *optarg);
void param_destroy(struct param_state *pstate);
void param_free(struct param_entry *p);

#endif

