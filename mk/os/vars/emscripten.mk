# this file is included by the main Makefile automatically
export EXE_SUFFIX = .js
EMCC = emcc
CC = $(EMCC)
JIT = 0
SDL = 0
USE_OWN_SEARCH = 1
DYLIB_SUFFIX = .js
CHECK_SW_TASKS = check_args check_behaviour check_compile check_sim check_obj # skip dogfood as it is quite expensive with emscripten

# Give $(MACHINE) an explicit value since it may not be computable (for
# example, when `emcc` is available only inside a Docker image)
MACHINE = asmjs-unknown-emscripten

EXPORTED_FUNCTIONS += main

DEFINES += 'MOUNT_POINT="/nodefs"'

# Avoid choking on stdin / stdout macros.
CFLAGS += -Wno-disabled-macro-expansion

ifeq ($(DEBUG),)
CC_OPT = -O2
CC_DEBUG =
CLOSURE_FLAGS = --closure 1
LDFLAGS += -s ASSERTIONS=0 \
           -s LIBRARY_DEBUG=0 \
           -s DISABLE_EXCEPTION_CATCHING=1 \
           -s WARN_ON_UNDEFINED_SYMBOLS=0 \
           -s ERROR_ON_UNDEFINED_SYMBOLS=1 \
           #

else
CC_OPT = -O0
CC_DEBUG = -g
CLOSURE_FLAGS = --closure 0
LDFLAGS += -s ASSERTIONS=2 \
           -s LIBRARY_DEBUG=0 \
           -s WARN_ON_UNDEFINED_SYMBOLS=1 \
           #
endif
null :=#
space :=$(null) $(null)#
comma :=$(null),#
LDFLAGS += -s "EXPORTED_FUNCTIONS=[$(subst $(space),$(comma),$(foreach f,$(EXPORTED_FUNCTIONS),'_$f'))]"

LDFLAGS += $(CC_OPT) $(CC_DEBUG)
LDFLAGS += $(CLOSURE_FLAGS)

clean_FILES += $(BUILDDIR)/*.js.mem $(BUILDDIR)/*.js $(BUILDDIR)/*.data

# runwrap might be nodejs on Debian
runwrap := node # trailing space required
