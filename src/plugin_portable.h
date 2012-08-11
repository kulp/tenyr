#ifndef PLUGIN_PORTABLE_H_
#define PLUGIN_PORTABLE_H_

// portable (non-OS-specific) plugin definitions

struct tenyr_plugin_ops {
    void (*fatal)(int code, const char *file, int line, const char *func, const char *fmt, ...);
    void (*debug)(int level, const char *file, int line, const char *func, const char *fmt, ...);
};

typedef void plugin_init(struct tenyr_plugin_ops *ops);

// this is called by plugin hosts
int tenyr_plugin_host_init(void *libhandle);

// signal to other includes that we are in plugin mode
#ifndef TENYR_PLUGIN
#define TENYR_PLUGIN 1
#endif

#endif

