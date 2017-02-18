#ifndef PLUGIN_PORTABLE_H_
#define PLUGIN_PORTABLE_H_

#include "plugin.h"

#include <stddef.h>
#include <stdint.h>

#define NORETURN __attribute__((noreturn))

// portable (non-OS-specific) plugin definitions

struct plugin_cookie;

struct guest_ops {
    void (* NORETURN fatal)(int code , const char *file, int line, const char *func, const char *fmt, ...);
    void (*          debug)(int level, const char *file, int line, const char *func, const char *fmt, ...);
    int  (*      param_get)(const struct plugin_cookie *cookie, const char *key, size_t count, const void **val);
    int  (*      param_set)(struct plugin_cookie *cookie, char *key, char *val, int replace, int free_key, int free_value);
};

struct plugin_cookie {
    struct guest_ops gops;
    struct param_state *param;
    char prefix[32];

    struct plugin_cookie *wrapped;
};

typedef int EXPORT_CALLING library_init(struct guest_ops *ops);

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

typedef int plugin_success_cb(void *libhandle, int inst, const char *parent,
        const char *implstem, void *ud);

int plugin_load(const char *basepath, const char **paths, const char *base,
        const struct plugin_cookie *p, plugin_success_cb *success, void *ud);

#endif

/* vi: set ts=4 sw=4 et: */
