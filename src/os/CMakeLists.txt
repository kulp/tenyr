add_subdirectory(default)

add_subdirectory(Darwin)
add_subdirectory(Linux)
add_subdirectory(Win32)

add_library(os_support INTERFACE)
target_link_libraries(os_support
    INTERFACE
        os_support_${CMAKE_SYSTEM_NAME}
)
