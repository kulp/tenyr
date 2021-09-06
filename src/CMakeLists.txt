configure_file(tenyr_config.h.in tenyr_config.h)

add_subdirectory(os)

add_library(common STATIC
    common.c
    param.c
    stream.c
)

target_include_directories(common PUBLIC .)
target_link_libraries(common PUBLIC os_support)

add_executable(tld
    tld.c

    obj.c
)
target_link_libraries(tld PUBLIC common)
