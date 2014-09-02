#ifndef PLUGIN_H_
#define PLUGIN_H_

// this header should be included before all other headers, by any files that
// are compiled into a plugin

// _GNU_SOURCE is needed for RTLD_DEFAULT on GNU/Linux, although that flag
// works on apple-darwin as well
#define _GNU_SOURCE 1

#include <dlfcn.h>
#define EXPORT
#define EXPORT_CALLING

#include "plugin_portable.h"

#endif

/* vi: set ts=4 sw=4 et: */
