#ifndef PLUGIN_PORTABLE_H_
#define PLUGIN_PORTABLE_H_

#include "plugin.h"

#include <stdint.h>

#define NORETURN __attribute__((noreturn))

struct plugin_cookie {
    struct param_state *param;
};

// portable (non-OS-specific) plugin definitions

struct guest_ops {
    void (* NORETURN fatal)(int code , const char *file, int line, const char *func, const char *fmt, ...);
    void (*          debug)(int level, const char *file, int line, const char *func, const char *fmt, ...);
    int  (*      param_get)(void *cookie, char *key, const char **val);
    int  (*      param_set)(void *cookie, char *key, char *val, int free_value);
};

typedef int EXPORT_CALLING library_init(struct guest_ops *ops);

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

typedef int plugin_success_cb(void *libhandle, const char *implstem, void *ud);

int plugin_load(const char *base, struct guest_ops *gops, void *hostcookie,
        plugin_success_cb *success, void *ud);

#endif

