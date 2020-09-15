TOP := ..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib

vpath %.tas pass_compile/ fail_compile/

get_obj_name = $(notdir $(patsubst %.tas,%.to,$1))

PASS_ASSEMBLE := $(call get_obj_name,$(wildcard pass_compile/*))
FAIL_ASSEMBLE := $(call get_obj_name,$(wildcard fail_compile/*))
# Force re-assembly of assembly-failure tests
.PHONY: $(FAIL_ASSEMBLE)

clean_FILES += *.to *.texe
clean_FILES += ops/*.texe

all: positive negative
positive: $(PASS_ASSEMBLE)
negative: $(FAIL_ASSEMBLE)

$(filter %.to,$(FAIL_ASSEMBLE)): %.to: %.tas
	@$(MAKESTEP) -n "Assembling `printf %-24s '$@'` (should fail) ... "
	@(! $(tas) -o$@ $< 2>/dev/null) \
		&& $(MAKESTEP) "failed as expected" \
		|| ($(MAKESTEP) "succeeded unexpectedly" ; false)

