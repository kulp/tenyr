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

        init(&ops);
    }

    return 0;
}

int plugin_load(const char *basepath, const char **paths, const char *base,
        const struct plugin_cookie *p, plugin_success_cb *success, void *ud)
{
    int rc = 0;

    const void *impls[16];
    int inst = 0;
    do {
        char buf[256], parent[256];
        snprintf(parent, sizeof parent, "%s[%d]", base, inst);
        int count = p->gops.param_get(p, parent, countof(impls), impls);
        if (count > (signed)countof(impls))
            debug(1, "Got %d impl names, only have room for %zd", count, countof(impls));
        else if (count <= 0)
            break;

        for (int i = 0; i < count; i++) {
            const char *implname = impls[i];
            // If implname contains a slash, treat it as a path ; otherwise, stem
            const char *implpath = NULL;
            const void *implstem = NULL;
            snprintf(buf, sizeof buf, "%s[%d].stem", base, inst);
            p->gops.param_get(p, buf, 1, &implstem); // may not be set ; that's OK
            if (strchr(implname, PATH_COMPONENT_SEPARATOR_CHAR)) {
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

            tenyr_plugin_host_init(libhandle);

            rc |= success(libhandle, inst, parent, implstem, ud);

            // if RTLD_NODELETE worked and were standard, we would dlclose() here
        }

        inst++;
    } while (!rc);

    return rc;
}
/* vi: set ts=4 sw=4 et: */
