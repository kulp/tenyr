libtenyrjit$(DYLIB_SUFFIX): LDLIBS += -llightning
libtenyrjit$(DYLIB_SUFFIX): param,dy.o
libtenyrjit$(DYLIB_SUFFIX): cjit,dy.o

ifneq ($(JIT),0)
LIB_TARGETS += libtenyrjit$(DYLIB_SUFFIX)
endif

CLEAN_FILES += libtenyrjit$(DYLIB_SUFFIX)

