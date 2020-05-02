makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(abspath $(dir $(makefile_path))/..)
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib
vpath %.tas.cpp $(TOP)/lib

vpath %.tas pass_compile/ fail_compile/

get_obj_name = $(notdir $(patsubst %.tas,%.to,$(patsubst %.tas.cpp,%.tas,$1)))

PASS_ASSEMBLE := $(call get_obj_name,$(wildcard pass_compile/*))
FAIL_ASSEMBLE := $(call get_obj_name,$(wildcard fail_compile/*))
# Force re-assembly of assembly-failure tests
.PHONY: $(FAIL_ASSEMBLE)

clean_FILES += *.to *.texe
clean_FILES += ops/*.texe

all: positive negative
positive: $(PASS_ASSEMBLE)
negative: $(FAIL_ASSEMBLE)

test_imul.texe: imul.to puts.to print_hex.to
copy.texe: memcpy.to puts.to

$(filter %.to,$(FAIL_ASSEMBLE)): %.to: %.tas
	@$(MAKESTEP) -n "Assembling `printf %-24s '$@'` (should fail) ... "
	@(! $(tas) -o$@ $< 2>$(DEVNUL)) \
		&& $(MAKESTEP) "failed as expected" \
		|| ($(MAKESTEP) "succeeded unexpectedly" ; false)

