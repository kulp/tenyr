#include "plugin.h"
#include "common.h"
#include "os_common.h"

#include <stdio.h>
#include <stdlib.h>

int tenyr_plugin_host_init(void *libhandle)
{
    void *ptr = dlsym(libhandle, "tenyr_plugin_init");
    library_init *init = ALIASING_CAST(library_init,ptr);
    if (init) {
        struct guest_ops ops = {
            .fatal = fatal_,
            .debug = debug_,
        };

        return init(&ops);
    }

    return 0;
}

int plugin_load(const char *basepath, const char **paths, const char *base,
        const struct plugin_cookie *p, plugin_success_cb *success, void *ud)
{
    int rc = 0;

    const int max_impls = 16;
    // We use a constant `16` here, instead of `max_impls`, to avoid making
    // `impls` a VLA (at least under some compilers) and thus making it illegal
    // to initialize it with an initializer list.
    const void *impls[16] = { 0 };
    int inst = 0;
    do {
        char buf[256] = { 0 }, parent[256] = { 0 };
        snprintf(parent, sizeof parent, "%s[%d]", base, inst);
        int count = p->gops.param_get(p, parent, max_impls, impls);
        if (count > max_impls)
            debug(0, "Saw %d plugins names, handling only the first %d", count, max_impls);
        else if (count <= 0)
            break;

        for (int i = 0; i < count && i < max_impls; i++) {
            const char *implname = impls[i];
            // If implname contains a slash, treat it as a path ; otherwise, stem
            const char *implpath = NULL;
            const void *implstem = NULL;
            snprintf(buf, sizeof buf, "%s[%d].stem", base, inst);
            p->gops.param_get(p, buf, 1, &implstem); // may not be set ; that's OK
            if (strchr(implname, '/')) {
                implpath = implname;
            } else {
                snprintf(buf, sizeof buf, "libtenyr%s"DYLIB_SUFFIX, implname);
                buf[sizeof buf - 1] = 0;
                implpath = buf;
                if (!implstem)
                    implstem = implname;
            }

            // TODO consider using RTLD_NODELETE here
            // (seems to break on Mac OS X)
            // currently we leak library handles
            const char **path = paths;
            void *libhandle = NULL;
            while ((libhandle == NULL) && *path != NULL) {
                char *resolved = build_path(basepath, "%s%s", *path++, implpath);
                void *handle = dlopen(resolved, RTLD_NOW | RTLD_LOCAL);
                if (handle) {
                    libhandle = handle;
                    debug(3, "handle %p for %s", handle, resolved);
                } else {
                    debug(3, "no handle for %s: %s", resolved, dlerror());
                }
                free(resolved);
            }

            int failed_init = tenyr_plugin_host_init(libhandle);
            if (failed_init)
                return failed_init;

            rc |= success(libhandle, inst, parent, implstem, ud);

            // if RTLD_NODELETE worked and were standard, we would dlclose() here
        }

        inst++;
    } while (!rc);

    return rc;
}
/* vi: set ts=4 sw=4 et: */
