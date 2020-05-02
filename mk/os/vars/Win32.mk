# this file is included by the main Makefile automatically
# TODO make this makefile work on win32
DYLIB_SUFFIX = .dll
# Forward slashes work well enough on Windows, and escaping backslashes is
# shell-dependent
PATH_COMPONENT_SEP=/
PATH_SEP_CHAR = ';'
EXE_SUFFIX = .exe
CFLAGS_PIC =
CC := gcc
ifeq ($(BITS),32)
 CROSS_COMPILE ?= i686-w64-mingw32-
else
 CROSS_COMPILE ?= x86_64-w64-mingw32-
endif
LDFLAGS += -Wl,--kill-at -static-libgcc -static
# JIT on Windows doesn't work yet
JIT = 0

sim.o tsim.o common.o: CPPFLAGS += -Wno-misleading-indentation

URL_SDL2_image = https://www.libsdl.org/projects/SDL_image/release/SDL2_image-devel-2.0.1-mingw.tar.gz
URL_SDL2       = https://www.libsdl.org/release/SDL2-devel-2.0.4-mingw.tar.gz

DL_DIR_SDL2       = $(TOP)/3rdparty/sdl2
DL_DIR_SDL2_image = $(TOP)/3rdparty/sdl2

SDL2_PKGCONFIG = PKG_CONFIG_PATH=$(TOP)/3rdparty/sdl2/$(MACHINE)/lib/pkgconfig pkg-config --define-variable=prefix=$(TOP)/3rdparty/sdl2/$(MACHINE)

# Create Wine path for a given UNIX path
os_path = $(if $(USE_WINE),z:$(subst /,\\,$1),$1)
runwrap = $(if $(USE_WINE),$(TOP)/scripts/winewrap $(runwrap))
