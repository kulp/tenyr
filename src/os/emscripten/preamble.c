#include "emscripten.h"
#include <stdlib.h>
#include <unistd.h>
#include "param.h"

int os_preamble(struct param_state *params)
{
    (void)params;
    if (access("/dev/zero", R_OK))
        EM_ASM({ FS.createDevice('/dev', 'zero', function () { return 0; }); });

    EM_ASM_({
        if (ENVIRONMENT_IS_NODE) {
            var mnt = Pointer_stringify($0);
            FS.mkdir(mnt);
            FS.mount(NODEFS, { root: '/' }, mnt);
            FS.chdir(mnt + process.cwd());
        }
    }, MOUNT_POINT);

    return 0;
}

