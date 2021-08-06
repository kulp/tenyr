makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := ..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

SHELL := $(shell which bash)

$(build_tas) $(build_tsim) $(build_tld):
	$(MAKE) -f $(TOP)/Makefile $@

INSTALL_DIR ?= /usr/local

local-install: INSTALL_DIR = $(TOP)/dist/$(MACHINE)
local-install: install

install:: $(BIN_TARGETS)
	install -d $(INSTALL_DIR)/bin
	install $^ $(INSTALL_DIR)/bin

install:: $(LIB_TARGETS)
	install -d $(INSTALL_DIR)/lib
	$(if $(LIB_TARGETS),install $^ $(INSTALL_DIR)/lib)

install:: $(RESOURCES)
	install -d $(subst $(TOP)/,$(INSTALL_DIR)/share/tenyr/,$(^D))
	$(foreach f,$^,install -m 0644 $f $(INSTALL_DIR)/share/tenyr/$(subst $(TOP)/,,$(dir $f));)

uninstall:: $(BIN_TARGETS)
	$(RM) $(foreach t,$(^F),$(INSTALL_DIR)/bin/$t)

uninstall:: $(LIB_TARGETS)
	$(RM) $(foreach t,$(^F),$(INSTALL_DIR)/lib/$t)

uninstall:: $(RESOURCES)
	$(RM) -r $(INSTALL_DIR)/share/tenyr

doc: tas_usage tsim_usage tld_usage

%_usage: %$(EXE_SUFFIX)
	@$(MAKESTEP) -n "Generating usage description for $* ... "
	exec -a $* $($*) --help | \
		sed -e 's/^/    /' \
		    -e '/version/s/-[0-9][0-9]*-g[[:xdigit:]]\{7\}/$1.../' \
	        > $(TOP)/wiki/$*--help.md && $(MAKESTEP) ok

gzip zip: export CREATE_TEMP_INSTALL_DIR=1

ifeq ($(CREATE_TEMP_INSTALL_DIR),1)
gzip: tenyr-$(BUILD_NAME).tar.gz
zip: tenyr-$(BUILD_NAME).zip

ZIPBALLS = tenyr-$(BUILD_NAME).zip tenyr-$(BUILD_NAME).tar.gz
$(ZIPBALLS): INSTALL_PRE := $(shell mktemp -d tenyr.dist.XXXXXX)
$(ZIPBALLS): INSTALL_DIR := $(INSTALL_PRE)/tenyr-$(BUILD_NAME)

tenyr-$(BUILD_NAME).tar.gz: install
	tar zcf $@ -C $(INSTALL_DIR)/.. .
	$(RM) -r $(INSTALL_PRE)

tenyr-$(BUILD_NAME).zip: install
	orig=$(abspath $@) && (cd $(INSTALL_DIR)/.. ; zip -r $$orig .)
	$(RM) -r $(INSTALL_PRE)

else
gzip zip:
	$(MAKE) -f $(makefile_path) $@
endif

coverage: coverage_html

LCOV ?= lcov
LCOVFLAGS = --config-file=$(TOP)/scripts/lcovrc

COVERAGE_RULE = check
coverage.info: LCOVFLAGS += --exclude="*/lexer.c"
coverage.info: LCOVFLAGS += --exclude="*/parser.c"
coverage.info: $(COVERAGE_RULE)
	$(LCOV) $(LCOVFLAGS) --capture --test-name $< --directory $(BUILDDIR) --output-file $@

coverage_html: coverage.info
	genhtml --output-directory $@ $^

check: check_sw
CHECK_SW_TASKS ?= check_obj
check_sw: check_ctest
check_sw: $(CHECK_SW_TASKS)

check_ctest: vpi jit
	cmake -S $(TOP) -B $(BUILDDIR)/ctest
	$(MAKE) --directory=$(TOP)/hw/icarus BUILDDIR=$(realpath $(BUILDDIR))
	cmake --build $(BUILDDIR)/ctest
	export PATH=$(abspath $(BUILDDIR)):$$PATH && cd $(BUILDDIR)/ctest && ctest

clean_FILES += check_obj_*.to ff.bin
ff.bin:; echo -ne '\0377\0377\0377\0377' > $@
check_obj: check_obj_0 check_obj_2 check_obj_4 check_obj_5 check_obj_6
check_obj_%: check_obj_%.to | $(build_tas)
	(! $(tas) -d $< 2> /dev/null)
check_obj_%.to: $(TOP)/test/misc/obj/null.to ff.bin
	cp $< $@
	dd bs=4 if=ff.bin of=$@ seek=$* 2>/dev/null
	dd bs=4 if=$< of=$@ skip=$$(($*+1)) seek=$$(($*+1)) 2>/dev/null

vpi:
	$(MAKE) -C $(BUILDDIR) -f $(TOP)/hw/vpi/Makefile $@

ifneq ($(JIT),0)
tsim_FLAGS_jit = -rjit -ptsim.jit.run_count_threshold=2
jit:
	$(MAKE) -f $(TOP)/Makefile libtenyrjit$(DYLIB_SUFFIX)
else
jit: ; # JIT support not enabled
endif

