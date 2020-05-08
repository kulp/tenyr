makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(dir $(makefile_path))
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

# ensure directory printing doesn't mess up check rules
GNUMAKEFLAGS += --no-print-directory

.DEFAULT_GOAL = all

CPPFLAGS += -'DDYLIB_SUFFIX="$(DYLIB_SUFFIX)"'

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

common_OBJECTS = common.o param.o $(patsubst %.c,%.o,$(notdir $(wildcard $(OS_PATHS:%=%/*.c)))) \
                 stream.o
shared_OBJECTS = common.o
tas_OBJECTS    = $(common_OBJECTS) asmif.o asm.o obj.o parser.o lexer.o
tsim_OBJECTS   = $(common_OBJECTS) simif.o asm.o obj.o plugin.o \
                 $(DEVOBJS) sim.o emscripten.o
tld_OBJECTS    = $(common_OBJECTS) obj.o

ifeq ($(USE_OWN_SEARCH),1)
# The interface of lsearch and tsearch is not something we can change.
lsearch.o tsearch.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)cast-qual

tas_OBJECTS   += lsearch.o tsearch.o
tld_OBJECTS   += lsearch.o tsearch.o
tsim_OBJECTS  += lsearch.o tsearch.o
endif

showbuilddir:
	@echo $(abspath $(BUILDDIR))

.PHONY: distclean
distclean:: clobber
	-$(RM) -r install/ build/ dist/

clean clobber::
	-rmdir $(BUILDDIR)/devices $(BUILDDIR) build # fail, ignore if non-empty
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
DROP_TARGETS = showbuilddir clean clobber distclean NODEPS
NODEPS:; # phony target just to prevent dependency generation
ifneq ($(BUILDDIR),.)
all $(filter-out $(DROP_TARGETS),$(MAKECMDGOALS))::
	mkdir -p $(BUILDDIR)
	$(MAKE) TOOLDIR=$(BUILDDIR) BUILDDIR=. -C $(BUILDDIR) -f $(makefile_path) TOP=$(TOP) $@
else

all: $(TARGETS) | $(TOP)/build/share/tenyr/rsrc

# Create symlinks for share-items in $(TOP)
$(TOP)/build/share/tenyr/%:
	@$(MAKESTEP) [ LN ] $*
	mkdir -p $(@D)
	ln -sf ../../../$* $@

-include $(TOP)/mk/os/rules/$(OS).mk

.PHONY: vpi
vpi: vpidevices.vpi
vpidevices.vpi: callbacks,dy.o vpiserial,dy.o load,dy.o sim,dy.o asm,dy.o obj,dy.o common,dy.o param,dy.o stream,dy.o

tas$(EXE_SUFFIX):  tas.o  $(tas_OBJECTS)
tsim$(EXE_SUFFIX): tsim.o $(tsim_OBJECTS)
tld$(EXE_SUFFIX):  tld.o  $(tld_OBJECTS)

# used to apply to .o only but some make versions built directly from .c
tas$(EXE_SUFFIX) tsim$(EXE_SUFFIX) tld$(EXE_SUFFIX): DEFINES += BUILD_NAME='$(BUILD_NAME)'

# Padding warnings are not really relevant to this project.
CFLAGS += -Wno-padded

# don't complain about unused state
asm.o asmif.o $(DEVOBJS) $(PDEVOBJS): CFLAGS += -Wno-unused-parameter
# link plugin-common data and functions into every plugin
$(PDEVLIBS): libtenyr%$(DYLIB_SUFFIX): pluginimpl,dy.o $(shared_OBJECTS:%.o=%,dy.o)

# Some casting away of qualifiers is currently deemed unavoidable, at least
# without running into different warnings.
tas.o tld.o param.o param,dy.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)cast-qual
# Calls to variadic printf functions almost always need non-literal format
# strings.
common.o parser.o stream.o: CFLAGS += -Wno-format-nonliteral

# We cannot control some aspects of the generated lexer or parser.
lexer.o: CFLAGS += -Wno-missing-prototypes
parser.o lexer.o: CFLAGS += -Wno-unused-macros
lexer.o: CFLAGS += -Wno-shorten-64-to-32
lexer.o: CFLAGS += -Wno-conversion
lexer.o: CPPFLAGS += -Wno-disabled-macro-expansion
lexer.o: CPPFLAGS += -Wno-documentation
lexer.o: CFLAGS += -Wno-missing-noreturn
parser.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)sign-conversion

# flex-generated code we can't control warnings of as easily
parser.o lexer.o: CFLAGS += -Wno-sign-compare -Wno-unused -Wno-unused-parameter
# flex-generated code needs POSIX source for fileno()
lexer.o: CPPFLAGS += -D_POSIX_SOURCE

# Do not warn about the unused version for endian reading.
obj.o obj,dy.o: CFLAGS += -Wno-unused-function

lexer.o asmif.o tas.o: parser.h
parser.h parser.c: lexer.h

ifeq ($(filter $(DROP_TARGETS),$(MAKECMDGOALS)),)
-include $(patsubst $(TOP)/src/%.c,$(BUILDDIR)/%.d,$(SOURCEFILES))
-include $(BUILDDIR)/lexer.d
-include $(BUILDDIR)/parser.d
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

