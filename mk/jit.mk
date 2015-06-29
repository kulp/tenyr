# TODO keep these changes from affecting Makefiles that include this one
CXXFLAGS += -fPIC

ASMJIT_BASE = $(TOP)/3rdparty/asmjit
ASMJIT_SRC = $(ASMJIT_BASE)/src
vpath %.cpp $(ASMJIT_SRC)/asmjit/base
vpath %.cpp $(ASMJIT_SRC)/asmjit/x86

ASMJIT_OBJS = \
    assembler.o codegen.o compiler.o constpool.o containers.o context.o \
    cpuinfo.o cputicks.o error.o globals.o intutil.o logger.o operand.o \
    runtime.o string.o vmem.o zone.o \
    x86assembler.o x86compiler.o x86context.o x86cpuinfo.o x86inst.o \
    x86operand.o x86operand_regs.o x86scheduler.o

# Suppress warnings on code we do not control
$(ASMJIT_OBJS): CXXFLAGS := $(filter-out -W%,$(CXXFLAGS))

libtenyrjit$(DYLIB_SUFFIX): CPPFLAGS += -I$(ASMJIT_SRC)
libtenyrjit$(DYLIB_SUFFIX): LDFLAGS  += -fPIC -shared
# Compensate for old glibc versions with -lrt
libtenyrjit$(DYLIB_SUFFIX): LDLIBS_Linux += -lrt
libtenyrjit$(DYLIB_SUFFIX): $(ASMJIT_OBJS)
libtenyrjit$(DYLIB_SUFFIX): param,dy.o

CLEAN_FILES += libtenyrjit$(DYLIB_SUFFIX)

