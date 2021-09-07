add_library(os_support_Linux
    findself.c
)

set_target_properties(os_support_Linux PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(os_support_Linux os_support_default)

target_include_directories(os_support_Linux
    PUBLIC
        .
)

target_compile_definitions(os_support_Linux
    INTERFACE
        DYLIB_SUFFIX=".so"
)
