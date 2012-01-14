/*
 * Defines a way to interact with the simulator programmatically and call
 * functions.
 */

#ifndef FFI_H_
#define FFI_H_

#include "sim.h"

#include <stdint.h>
#include <stddef.h>

// continue? predicate : returns nonzero to stop execution
typedef int cont_pred(struct mstate *m);

// TODO integrate with real objects from obj branch
struct obj {
    uint32_t base;
    size_t allocated;
    size_t used;
    char *data;
};

// tf prefix = tenyr "ffi"
int tf_read_file(struct obj *o, const char *filename);
int tf_load_obj(struct state *s, const struct obj *o);
int tf_run_until(struct state *s, uint32_t start_address, int flags, cont_pred stop);
int tf_get_addr(const /*?*/ struct state *s, const char *symbol, uint32_t *addr);
int tf_call(struct state *s, const char *symbol);

#endif

