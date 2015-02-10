# this file is included by the main Makefile automatically
DYLIB_SUFFIX = .dylib
# these symbols are loaded from the hosting executable
LDFLAGS += -Wl,-U,_fatal_
