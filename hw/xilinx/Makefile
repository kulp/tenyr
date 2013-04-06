TAS = ../../tas
TLD = ../../tld
SHELL = /bin/bash
SETUP = source ~/ise.sh
TARGET = Tenyr

BURNER = fpgalink
FLCLI = flcli

all: $(BURNTARGET)

ifeq ($(BURNER),fpgalink)
BURNTARGET = $(TARGET).xsvf
burn: $(BURNTARGET)
	$(FLCLI) -v 1443:0007 -i 1443:0007 -x $<
else
BURNTARGET = $(TARGET).bit
burn: $(BURNTARGET)
	djtgcfg -d Nexys3 -i 0 -f $< prog
endif

vpath %.tas ../verilog
vpath %.tas.cpp ../verilog

CPPFLAGS += -I../../lib

%.xsvf: %.bit burn.batch.tmpl
	sed -e 's/{TOP_LEVEL}/$*/g' burn.batch.tmpl > $@.batch.$$$$ ; \
	$(SETUP) && impact -batch $@.batch.$$$$ ;\
	rm $@.batch.$$$$

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) -o $@ $<

%.to: %.tas
	$(TAS) -o$@ $<

%.tasd: %.to
	$(TAS) -d -o$@ $<

%.tasd: %.mif
	perl -nle 'printf "0x%08x\n", unpack("N", pack("B32", $$_))' < $< | $(TAS) -ftext -d -o $@ -

%.texe: %.to
	$(TLD) -o$@ $^

vpath %.coe ipcore_dir
%.coe: %.texe
	echo 'memory_initialization_radix = 16;' > $@
	echo 'memory_initialization_vector =' >> $@
	$(TAS) -d $< | sed 's/.*# \.word //; s/ ;.*//; s/^0x//' >> $@
	echo ';' >> $@
	mv $@ ipcore_dir

vpath gen_%.tcl ipcore_dir
vpath %.xco ipcore_dir
vpath %.ngc ipcore_dir

%.ngc: gen_%.tcl
	$(SETUP) && cd $(dir $<) && xtclsh $(notdir $<)

GenedBlockMem.ngc: GenedBlockMem.xco small.coe

vpath %.v ../verilog
vpath %.vh ../verilog

Tenyr_SRCDEPS += \
	common.vh \
	fontram.xco \
	GenedBlockMem.xco \
	hex2ascii.v \
	mmr.v \
	Nexys3_Master.ucf \
	ramwrap.v \
	seg7.v \
	seglookup.v \
	tenyr_mainclock.xco \
	Tenyr.ucf \
	tenyr.v \
	textram.xco \
	vga80x40_col1fixed.vhd \
	vga80x40.ucf \
	xilinx_top.v \
# end of Tenyr_SRCDEPS list

# TODO pull tenyr.tcl components into Makefile to allow incremental builds
%.bit: $($*_SRCDEPS) GenedBlockMem.ngc
	$(SETUP) && xtclsh $*.tcl run_process

clean:
	@echo 'Run `make clobber` to clean up generated files'

clobber:
	xargs rm -rf < .gitignore
	git ls-files --others | xargs rm -rf
