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

find_package(FLEX)
flex_target(scanner
    lexer.l lexer.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/lexer.h
)

find_package(BISON 3.7.6)
bison_target(parser
    parser.y parser.c
    DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h
)

add_flex_bison_dependency(scanner parser)
