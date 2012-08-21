#ifndef PLUGIN_PORTABLE_H_
#define PLUGIN_PORTABLE_H_

#include "plugin.h"

#include <stdint.h>

#define NORETURN __attribute__((noreturn))

// portable (non-OS-specific) plugin definitions

struct guest_ops {
    void (* NORETURN fatal)(int code , const char *file, int line, const char *func, const char *fmt, ...);
    void (*          debug)(int level, const char *file, int line, const char *func, const char *fmt, ...);
};

typedef int EXPORT_CALLING library_init(struct guest_ops *ops);
typedef int EXPORT_CALLING plugin_init(void *pcookie);
typedef int EXPORT_CALLING plugin_mem_op(void *ud, int op, uint32_t addr, uint32_t *data);
typedef int EXPORT_CALLING plugin_fini(void *cookie);

struct plugin_ops {
    plugin_init     *init;
    plugin_mem_op   *mem_op;
    plugin_fini     *fini;
} ops;

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

// signal to other includes that we are in plugin mode
#ifndef TENYR_PLUGIN
#define TENYR_PLUGIN 1
#endif

#endif

