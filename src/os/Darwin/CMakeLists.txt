add_library(os_support_Darwin
    findself.c
)

set_target_properties(os_support_Darwin PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_link_libraries(os_support_Darwin os_support_default)

target_include_directories(os_support_Darwin
    PUBLIC
        .
)

target_compile_definitions(os_support_Darwin
    INTERFACE
        DYLIB_SUFFIX=".so"
)
