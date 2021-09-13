if(NOT ICARUS)
    return()
endif()

add_library(vpidevices
    MODULE

    callbacks.c
    load.c
    vpidevices.c
    vpiserial.c

    ${CMAKE_SOURCE_DIR}/src/sim.c
    ${CMAKE_SOURCE_DIR}/src/asm.c
    ${CMAKE_SOURCE_DIR}/src/obj.c
)

set_target_properties(vpidevices PROPERTIES PREFIX "" SUFFIX .vpi)

target_link_libraries(vpidevices PRIVATE common-pic)

execute_process(
    COMMAND iverilog-vpi --cflags
    OUTPUT_VARIABLE VPI_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(VPI_CFLAGS)
target_compile_options(vpidevices PRIVATE ${VPI_CFLAGS})

execute_process(
    COMMAND iverilog-vpi --ldflags
    OUTPUT_VARIABLE VPI_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(VPI_LDFLAGS)
target_link_options(vpidevices PRIVATE ${VPI_LDFLAGS})

execute_process(
    COMMAND iverilog-vpi --ldlibs
    OUTPUT_VARIABLE VPI_LDLIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
separate_arguments(VPI_LDLIBS)
target_link_libraries(vpidevices PRIVATE ${VPI_LDLIBS})
