makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(abspath $(dir $(makefile_path))/..)
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

.SUFFIXES:
.SUFFIXES: .tas .to .texe

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib
vpath %.tas.cpp $(TOP)/lib

vpath %.tas pass_compile/ fail_compile/

get_obj_name = $(notdir $(patsubst %.tas,%.to,$(patsubst %.tas.cpp,%.tas,$1)))

# TODO run .texe's instead of just compiling them
PASS_ASSEMBLE := \
	$(call get_obj_name,$(wildcard pass_compile/*)) \
	copy.texe \
	jzrel.texe \
	test_imul.texe \
	#

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
	@$(ECHO) -n "Assembling $@ (should fail) ... "
	@(! $(TAS) -o$@ $< 2>/dev/null) \
		&& $(ECHO) "failed as expected" \
		|| ($(ECHO) "succeeded unexpectedly" ; false)

