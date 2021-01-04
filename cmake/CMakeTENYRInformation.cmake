set(CMAKE_TENYR_COMPILE_OBJECT "<CMAKE_TENYR_COMPILER> <SOURCE> -o <OBJECT>")
# CMake wants to call a compiler front-end (CMAKE_TENYR_COMPILER) for linking,
# but we need a separate linker. For now, we interpolate the CMAKE_TENYR_LINKER
# directly into the string below.
set(CMAKE_TENYR_LINK_EXECUTABLE "${CMAKE_TENYR_LINKER} <OBJECTS> -o <TARGET>")
set(CMAKE_TENYR_INFORMATION_LOADED 1)
