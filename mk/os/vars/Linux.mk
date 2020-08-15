# this file is included by the main Makefile automatically
tsim$(EXE_SUFFIX) tsim-static$(EXE_SUFFIX): LDLIBS += -ldl
# We cannot build static binaries when SANITIZE is set
BIN_TARGETS += $(if $(SANITIZE),,$(BIN_TARGETS_STATIC))
