# Verilator-based simulator target.
if(NOT VERILATOR)
    return()
endif()
find_program(VERILATOR_BIN verilator)
if(NOT VERILATOR_BIN)
    message(FATAL_ERROR "Verilator not found. Install it or set VERILATOR=OFF")
endif()

# Generate C++ model from Verilog sources
set(VFILES
    ${CMAKE_SOURCE_DIR}/hw/verilog/top.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/sim/simclocks.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/tenyr.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/ram.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/seg7.v
    ${CMAKE_CURRENT_SOURCE_DIR}/simserial.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/hex2segments.v
    ${CMAKE_SOURCE_DIR}/hw/verilog/gpio.v
    ${CMAKE_SOURCE_DIR}/3rdparty/wb_intercon/rtl/verilog/wb_mux.v
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_main.cpp
)

set(VFLAGS
    -Wall -Wno-fatal -Wno-style
    -I${CMAKE_SOURCE_DIR}/hw/verilog
    --cc --trace
    -DSIMCLK=tenyr_mainclock
    -DSERIAL
    --Mdir ${CMAKE_CURRENT_BINARY_DIR}
    --exe
    --vpi --public-flat-rw
    -CFLAGS "-I${CMAKE_SOURCE_DIR}/src -I${CMAKE_BINARY_DIR}/src -I${CMAKE_SOURCE_DIR}/src/os/default"
    -LDFLAGS "-L${CMAKE_CURRENT_BINARY_DIR} -ldisasm -L${CMAKE_BINARY_DIR}/src -lcommon -lm"
)

# macOS: C++ standard library include path
if(APPLE)
    execute_process(
        COMMAND xcrun --show-sdk-path
        OUTPUT_VARIABLE SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    list(APPEND VFLAGS -CFLAGS "-isysroot${SDK_PATH} -I${SDK_PATH}/usr/include/c++/v1")
endif()


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/vsim
    COMMAND ${VERILATOR_BIN} ${VFLAGS} ${VFILES}
    COMMAND make -C ${CMAKE_CURRENT_BINARY_DIR} -f ${CMAKE_CURRENT_BINARY_DIR}/Vtop.mk Vtop
    COMMAND ${CMAKE_COMMAND} -E rename Vtop vsim
    DEPENDS ${VFILES} common disasm
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building Verilator simulator (vsim)"
)

add_custom_target(vsim ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/vsim
)

# Convenience: run the simulator
add_custom_target(vsim-run
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/vsim ${vsim_args}
    DEPENDS vsim
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
