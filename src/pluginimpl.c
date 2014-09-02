#include "plugin.h"

// These are the implementations of the `common` functions, plugin version.
void (* NORETURN fatal_)(int code, const char *file, int line, const char *func, const char *fmt, ...);
void (*          debug_)(int level, const char *file, int line, const char *func, const char *fmt, ...);

/* vi: set ts=4 sw=4 et: */
