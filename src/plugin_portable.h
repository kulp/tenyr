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

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

#endif

