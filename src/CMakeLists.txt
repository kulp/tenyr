configure_file(tenyr_config.h.in tenyr_config.h)

add_subdirectory(devices)
add_subdirectory(os)

add_library(common STATIC
    common.c
    param.c
    stream.c
)

target_include_directories(common PUBLIC .)
target_link_libraries(common PUBLIC os_support)

add_library(common-pic STATIC $<TARGET_PROPERTY:common,SOURCES>)
set_target_properties(common-pic PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_link_libraries(common-pic PUBLIC common)

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

add_executable(tas
    tas.c

    asm.c
    asmif.c
    obj.c
    ${FLEX_scanner_OUTPUTS}
    ${BISON_parser_OUTPUTS}
)

target_include_directories(tas PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(tas PUBLIC common)

add_executable(tsim
    tsim.c

    asm.c
    obj.c
    plugin.c
    sim.c
    simif.c

    devices/ram.c
    devices/serial.c
    devices/sparseram.c
    devices/zero_word.c
)

target_link_libraries(tsim PUBLIC common)
# Plugin support needs -ldl.
target_link_libraries(tsim PRIVATE dl)

if(JIT)
    add_library(tenyrjit MODULE jit.c cjit.c)
    target_link_libraries(tenyrjit PUBLIC common-pic)
    target_link_libraries(tenyrjit PRIVATE lightning)
endif()
