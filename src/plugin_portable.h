#ifndef PLUGIN_PORTABLE_H_
#define PLUGIN_PORTABLE_H_

#include "plugin.h"

#include <stdint.h>

#define NORETURN __attribute__((noreturn))

// portable (non-OS-specific) plugin definitions

struct guest_ops {
    void (* NORETURN fatal)(int code , const char *file, int line, const char *func, const char *fmt, ...);
    void (*          debug)(int level, const char *file, int line, const char *func, const char *fmt, ...);
    int  (*      param_get)(void *cookie, char *key, const char **val);
    int  (*      param_set)(void *cookie, char *key, char *val, int free_value);
};

struct plugin_cookie {
    void *cookie;
    struct guest_ops gops;
    struct param_state *param;

    struct plugin_cookie *parent;
};

typedef int EXPORT_CALLING library_init(struct guest_ops *ops);

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

typedef int plugin_success_cb(void *libhandle, int inst, const char *implstem,
        void *ud);

int plugin_load(const char *base, const struct plugin_cookie *p,
        plugin_success_cb *success, void *ud);

#endif

