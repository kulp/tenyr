# this file is included by the main Makefile automatically
CFLAGS_PIC += -fPIC
PATH_SEP_CHAR=':'
DYLIB_SUFFIX = .so
STLIB_SUFFIX = .a
EXE_SUFFIX =
# Respect SDL2_CONFIG from environment
SDL2_CONFIG ?= sdl2-config
# Pass through path unchanged in default configuration
os_path = $1
