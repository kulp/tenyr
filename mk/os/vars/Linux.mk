# this file is included by the main Makefile automatically
tsim$(EXE_SUFFIX): LDLIBS += -ldl
tsim$(EXE_SUFFIX): LDFLAGS += -rdynamic
