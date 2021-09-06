add_library(os_support_default
    findself.c
    linebuf.c
    open.c
    pagesize.c
    preamble.c
    tsimrc.c
)

target_compile_definitions(os_support_default
    PRIVATE
        PATH_SEPARATOR_CHAR=':'
)

target_include_directories(os_support_default
    PUBLIC
        .
)
