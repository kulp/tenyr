#include "emscripten.h"
#include <stdlib.h>
#include <unistd.h>
#include "param.h"

#include "os_common.h"

int os_preamble(void)
{
    if (access("/dev/zero", R_OK))
        EM_ASM(({ FS.createDevice('/dev', 'zero', function () { return 0; }); }),/* dummy arg */0);

    return 0;
}

