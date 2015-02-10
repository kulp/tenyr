# this file is included by the main Makefile automatically
# TODO make this makefile work on win32
DYLIB_SUFFIX = .dll
PATH_SEP_CHAR = '\\\\'
EXE_SUFFIX = .exe
CFLAGS_PIC =
CC := gcc
ifeq ($(_32BIT),1)
 CROSS_COMPILE ?= i686-w64-mingw32-
else
 # lfind() complains about size_t* vs. unsigned int* ; ignore for now
 PEDANTIC =
 CROSS_COMPILE ?= x86_64-w64-mingw32-
endif
LDFLAGS += -Wl,--kill-at
SDL = 0 # haven't tried SDL Windows build yet
