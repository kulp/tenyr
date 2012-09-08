#include "plugin.h"
#include "common.h"

#include <stdio.h>

int tenyr_plugin_host_init(void *libhandle)
{
    void *ptr = dlsym(libhandle, "tenyr_plugin_init");
    library_init *init = ALIASING_CAST(library_init,ptr);
    if (init) {
        struct guest_ops ops = {
            .fatal = fatal_,
            .debug = debug_,
        };

        init(&ops);
    }

    return 0;
}

int plugin_load(const char *base, struct guest_ops *gops, void *hostcookie,
        plugin_success_cb *success, void *ud)
{
    int rc = 0;

    const char *implname = NULL;
    int inst = 0;
    do {
        char buf[256];
        snprintf(buf, sizeof buf, "%s.impl[%d]", base, inst);
        if (gops->param_get(hostcookie, buf, &implname)) {
            // If implname contains a slash, treat it as a path ; otherwise, stem
            const char *implpath = NULL;
            const char *implstem = NULL;
            snprintf(buf, sizeof buf, "%s.impl[%d].stem", base, inst);
            gops->param_get(hostcookie, buf, &implstem); // may not be set ; that's OK
            if (strchr(implname, PATH_SEPARATOR_CHAR)) {
                implpath = implname;
            } else {
                snprintf(buf, sizeof buf, ".%clib%s"DYLIB_SUFFIX,
                        PATH_SEPARATOR_CHAR, implname);
                buf[sizeof buf - 1] = 0;
                implpath = buf;
                if (!implstem)
                    implstem = implname;
            }

            // TODO consider using RTLD_NODELETE here
            // (seems to break on Mac OS X)
            // currently we leak library handles
            void *libhandle = dlopen(implpath, RTLD_NOW | RTLD_LOCAL);
            if (!libhandle) {
                debug(1, "Could not load %s, trying default library search", implpath);
                libhandle = RTLD_DEFAULT;
            }

            tenyr_plugin_host_init(libhandle);

            success(libhandle, implstem, ud);

            // if RTLD_NODELETE worked and were standard, we would dlclose() here
        } else break;

        inst++;
    } while (!rc);

    return rc;
}
