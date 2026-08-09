# On Emscripten, these files override the default OS support implementations.
# Because os_support_Emscripten links against os_support_default and appears
# earlier in the link order, the linker resolves symbols (os_find_self,
# os_fopen, os_preamble, recipe_emscript) from this library first.

add_library(os_support_Emscripten
    emscripten.c
    findself.c
    open.c
    preamble.c
)

set_target_properties(os_support_Emscripten PROPERTIES EXCLUDE_FROM_ALL TRUE)

target_link_libraries(os_support_Emscripten os_support_default)

target_include_directories(os_support_Emscripten
    PUBLIC
        .
)

target_compile_definitions(os_support_Emscripten
    INTERFACE
        DYLIB_SUFFIX=".so"
)

# Emscripten's default filesystem is MEMFS.  When running under Node.js
# (e.g. for testing on the command line) os_preamble() mounts the real
# filesystem under this mount point so absolute paths resolve correctly.
target_compile_definitions(os_support_Emscripten
    PUBLIC
        MOUNT_POINT="/nodefs"
)
