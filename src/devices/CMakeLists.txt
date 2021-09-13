list(APPEND FAILURE_TESTS NONE INIT FINI OP PUMP PLUGIN_INIT ADD_DEVICE NO_ADD_DEVICE)
foreach(f ${FAILURE_TESTS})
    set(target tenyrfailure_${f})
    add_library(${target} MODULE failure.c)
    target_compile_definitions(
        ${target}
        PRIVATE
            "FAILURE_${f}=ENOTSUP"
            "FAILURE_ADD_DEVICE_FUNC=failure_${f}_add_device"
    )
    target_link_libraries(${target} PRIVATE common-pic)
    # These libraries are placed alongside the `tsim` binary in order to be
    # found.
    set_target_properties(
        ${target}
        PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src
    )
endforeach(f)

# Override default symbols for one target.
target_compile_options(
    tenyrfailure_NO_ADD_DEVICE
    PRIVATE
        -UFAILURE_ADD_DEVICE_FUNC
        -DFAILURE_ADD_DEVICE_FUNC=unfindable_function_name
)
