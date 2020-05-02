# this file is included by the main Makefile automatically
CFLAGS_PIC += -fPIC
PATH_COMPONENT_SEP = /
PATH_SEP_CHAR=':'
DYLIB_SUFFIX = .so
EXE_SUFFIX =
# We spell "DEVNUL" with one "L" only to make `$(DEVNUL)` fit in the same space
# as `/dev/null` used. This could be changed at any time by someone who cares
# to fix up the whitespace alignment issues that result.
DEVNUL = /dev/null
# Respect SDL2_PKGCONFIG from environment
SDL2_PKGCONFIG ?= pkg-config
# Pass through path unchanged in default configuration
os_path = $1
