# this file is included by the main Makefile automatically
# TODO make this makefile work on win32
DYLIB_SUFFIX = .dll
PATH_COMPONENT_SEP=\\\\
PATH_SEP_CHAR = ';'
EXE_SUFFIX = .exe
CFLAGS_PIC =
CC := gcc
ifeq ($(BITS),32)
 CROSS_COMPILE ?= i686-w64-mingw32-
else
 # lfind() complains about size_t* vs. unsigned int* ; ignore for now
 PEDANTIC =
 CROSS_COMPILE ?= x86_64-w64-mingw32-
endif
LDFLAGS += -Wl,--kill-at -static-libgcc -static-libstdc++
# haven't tried SDL Windows build yet
SDL = 0

check_sw_pre_Win32::
	@$(MAKESTEP) "Registering EXE files with binfmt_misc ..."
	$(SILENCE)[[ -e /proc/sys/fs/binfmt_misc/tenyr_wine ]] || echo ':tenyr_wine:M::MZ::'$(TOP)/scripts/winewrap: | sudo tee /proc/sys/fs/binfmt_misc/register > /dev/null
