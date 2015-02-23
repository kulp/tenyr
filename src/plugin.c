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

int plugin_load(const char *path, const char *base,
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
#if EMSCRIPTEN
            // emscripten doesn't support RTLD_DEFAULT
            debug(1, "Could not load %s, bailing", implpath);
            break;
#else
            if (!libhandle && path) {
                char buf[256];
                snprintf(buf, sizeof buf, "%s%c%s",
                         path, PATH_SEPARATOR_CHAR, implpath);
                libhandle = dlopen(buf, RTLD_NOW | RTLD_LOCAL);
            }

            if (!libhandle) {
                debug(1, "Could not load %s, trying default library search", implpath);
                libhandle = RTLD_DEFAULT;
            }
#endif

            tenyr_plugin_host_init(libhandle);

            success(libhandle, inst, parent, implstem, ud);

            // if RTLD_NODELETE worked and were standard, we would dlclose() here
        }

        inst++;
    } while (!rc);

    return rc;
}
/* vi: set ts=4 sw=4 et: */
