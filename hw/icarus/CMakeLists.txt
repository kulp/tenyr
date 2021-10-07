if(NOT ICARUS)
    return()
endif()

set(tenyr_DEPENDENCIES
    ${CMAKE_SOURCE_DIR}/3rdparty/wb_intercon/rtl/verilog/wb_mux.v
    ${CMAKE_SOURCE_DIR}/hw/icarus/simserial.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/gpio.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/hex2segments.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/ram.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/seg7.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/sim/simclocks.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/sim/simtop.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/tenyr.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/top.v
)

# We use IVERILOG_VPI_MODULE_PATH rather than -L in order to maintain backward
# compatibility with Icarus v10.
set(ENV{IVERILOG_VPI_MODULE_PATH} ${CMAKE_BINARY_DIR}/hw/vpi)

set(IVERILOG_FLAGS
    -m ${CMAKE_BINARY_DIR}/hw/vpi/vpidevices

    -DSIMCLK=tenyr_mainclock
    -DSERIAL
    -DSIM

    -I${CMAKE_SOURCE_DIR}/hw/verilog
    -g2005-sv
)

add_custom_command(
    OUTPUT  tenyr
    COMMAND iverilog ${IVERILOG_FLAGS} -Wall -o tenyr ${tenyr_DEPENDENCIES}
    DEPENDS ${tenyr_DEPENDENCIES} vpidevices
)

add_custom_target(icarus ALL DEPENDS tenyr)
