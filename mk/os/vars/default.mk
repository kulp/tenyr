# this file is included by the main Makefile automatically
CFLAGS_PIC += -fPIC
PATH_COMPONENT_SEP = /
PATH_SEP_CHAR=':'
DYLIB_SUFFIX = .so
STLIB_SUFFIX = .a
EXE_SUFFIX =
# Respect SDL2_PKGCONFIG from environment
SDL2_PKGCONFIG ?= pkg-config
# Pass through path unchanged in default configuration
os_path = $1
