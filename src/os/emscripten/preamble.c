#include "emscripten.h"
#include <stdlib.h>
#include <unistd.h>
#include "param.h"

int os_preamble(struct param_state *params)
{
    char *here = NULL;

    if (access("/dev/zero", R_OK))
        EM_ASM({ FS.createDevice('/dev', 'zero', function () { return 0; }); });
    if (param_get(params, "paths.cwd", 1, (const void **)&here) > 0) {
        EM_ASM_({
            var mnt = Pointer_stringify($0);
            var cwd = Pointer_stringify($1);
            FS.mkdir(mnt);
            FS.mount(NODEFS, { root: '/' }, mnt);
        }, MOUNT_POINT, here);
        if (chdir(MOUNT_POINT))
            perror("chdir");
        if (chdir(&here[1])) // skip leading slash to make a relative path
            perror("chdir");
        return 0;
    }

    return -1;
}

