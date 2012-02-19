/*
 * Defines a way to interact with the simulator programmatically and call
 * functions.
 */

#ifndef FFI_H_
#define FFI_H_

#include <stdint.h>
#include <stddef.h>

/// run first instruction unconditionally ?
#define TF_IGNORE_FIRST_PREDICATE 1

struct state;
struct mstate;
struct obj;

// continue? predicate : returns nonzero to stop execution
typedef int cont_pred(struct mstate *m, void *cud);

// tf prefix = tenyr `ffi' -> TODO change to just `sim' ?
int tf_load_obj(struct state *s, const struct obj *o);
int tf_run_until(struct state *s, uint32_t start_address, int flags,
        cont_pred stop, void *cud);
int tf_get_addr(const /*?*/ struct state *s, const char *symbol, uint32_t *addr);
int tf_call(struct state *s, const char *symbol);

#endif

