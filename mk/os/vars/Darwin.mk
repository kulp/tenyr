# this file is included by the main Makefile automatically
DYLIB_SUFFIX = .dylib
# these symbols are loaded from the hosting executable
LDFLAGS += -Wl,-U,_fatal_,-U,_debug_

ifeq ($(ARCH),arm64)
JIT ?= 0$(warning \
    JIT support is disabled due to lack of support on arch $(ARCH). \
    Set JIT=0 in your environment to suppress this warning, or set JIT=1 \
    if you want to try building the JIT anyway.)
endif
