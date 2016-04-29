makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

# ensure directory printing doesn't mess up check rules
GNUMAKEFLAGS += --no-print-directory

.DEFAULT_GOAL = all

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'
# Use := to ensure the expensive underlying call is not repeated
NO_UNKNOWN_WARN_OPTS := $(call cc_flag_supp,-Wno-unknown-warning-option)
CPPFLAGS += $(NO_UNKNOWN_WARN_OPTS)

SOURCEFILES = $(wildcard $(TOP)/src/*.c $(TOP)/src/devices/*.c)

VPATH += $(TOP)/src $(TOP)/src/devices $(TOP)/hw/vpi
INCLUDES += $(TOP)/src $(INCLUDE_OS) $(BUILDDIR)

clean_FILES = $(addprefix $(BUILDDIR)/,  \
                   *.o                   \
                   *.d                   \
                   parser.[ch]           \
                   lexer.[ch]            \
                   $(TARGETS)            \
                   $(SOURCEFILES:$(TOP)/src/%.c=%.d) \
                   random random.*       \
               )#

common_OBJECTS = common.o $(patsubst %.c,%.o,$(notdir $(wildcard $(OS_PATHS:%=%/*.c))))
tas_OBJECTS    = $(common_OBJECTS) asmif.o asm.o obj.o parser.o lexer.o param.o
tsim_OBJECTS   = $(common_OBJECTS) simif.o asm.o obj.o plugin.o \
                 $(DEVOBJS) sim.o param.o
tld_OBJECTS    = $(common_OBJECTS) obj.o

showbuilddir:
	@echo $(abspath $(BUILDDIR))

.PHONY: distclean
distclean:: clobber
	-$(RM) -r install/ build/ dist/

clean clobber::
	-rmdir $(BUILDDIR) build # fail, ignore if non-empty
	-$(MAKE) -C $(TOP)/test $@
	-$(MAKE) -C $(TOP)/forth $@
	-$(MAKE) -C $(TOP)/hw/icarus $@
	-$(MAKE) -C $(TOP)/hw/xilinx $@

clobber_FILES += $(BUILDDIR)/*.gc??
clobber_FILES += $(BUILDDIR)/coverage.info*
clobber_FILES += $(BUILDDIR)/PERIODS.mk
clobber_FILES += $(BUILDDIR)/coverage_html_*
clobber_FILES += $(BUILDDIR)/vpidevices.vpi
clobber_FILES += $(TOP)/test/op/*.texe
clobber_FILES += $(TOP)/test/run/*.texe
clobber::
	-$(MAKE) -C $(TOP)/ex $@

################################################################################
# Rerun make inside $(BUILDDIR) if we are not already building in the $(PWD)
DROP_TARGETS = win% showbuilddir clean clobber distclean
ifneq ($(BUILDDIR),.)
all $(filter-out $(DROP_TARGETS),$(MAKECMDGOALS))::
	mkdir -p $(BUILDDIR)
	$(MAKE) TOOLDIR=$(BUILDDIR) BUILDDIR=. -C $(BUILDDIR) -f $(makefile_path) TOP=$(TOP) $@
else

all: $(TARGETS)

.PHONY: vpi
vpi: vpidevices.vpi
vpidevices.vpi: callbacks,dy.o vpiserial,dy.o load,dy.o sim,dy.o asm,dy.o obj,dy.o common,dy.o param,dy.o

tas$(EXE_SUFFIX):  tas.o  $(tas_OBJECTS)
tsim$(EXE_SUFFIX): tsim.o $(tsim_OBJECTS)
tld$(EXE_SUFFIX):  tld.o  $(tld_OBJECTS)

asm.o: CFLAGS += -Wno-override-init

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

# don't complain about unused values that we might use in asserts
tas.o asm.o tsim.o sim.o simif.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-value
# don't complain about unused state
asm.o asmif.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-parameter
# link plugin-common data and functions into every plugin
$(PDEVLIBS): libtenyr%$(DYLIB_SUFFIX): pluginimpl,dy.o plugin,dy.o $(common_OBJECTS:%.o=%,dy.o)

# flex-generated code we can't control warnings of as easily
parser.o lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter

lexer.o asmif.o tas.o: parser.h
parser.h parser.c: lexer.h

ifeq ($(filter $(DROP_TARGETS),$(MAKECMDGOALS)),)
-include $(patsubst $(TOP)/src/%.c,$(BUILDDIR)/%.d,$(SOURCEFILES))
endif

# Dispatch all other known targets to auxiliary makefile
# We make them depend on `all` because it assuages some issues with doing a
# top-level `make -j check` (for example) where many copies of tas (for
# example) are built simultaneously, sometimes overwriting each other
# non-atomically
coverage: export GCOV=1
install local-install uninstall doc gzip zip coverage check check_sw check_hw check_sim check_jit check_compile dogfood: all
	$(MAKE) -f $(TOP)/mk/misc.mk $@

endif # BUILDDIR

