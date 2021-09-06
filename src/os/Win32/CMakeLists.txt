add_library(os_support_Win32
    findself.c
    linebuf.c
    pagesize.c
    tsimrc.c
)

set_target_properties(os_support_Win32 PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(os_support_Win32 os_support_default)

target_include_directories(os_support_Win32
    PUBLIC
        .
)

target_compile_definitions(os_support_Win32
    PRIVATE
        PATH_SEPARATOR_CHAR=';'
)
