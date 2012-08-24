TAS = ../../tas
TLD = ../../tld
SHELL = /bin/bash
SETUP = source ~/ise.sh
TARGET = Tenyr

all: $(TARGET).bit

burn: $(TARGET).bit
	djtgcfg -d Nexys3 -i 0 -f $< prog

vpath %.tas ../verilog
vpath %.tas.cpp ../verilog

CPPFLAGS += -I../../lib

%.tas: %.tas.cpp
	$(CPP) $(CPPFLAGS) -o $@ $<

%.to: %.tas
	$(TAS) -o$@ $<

%.tasd: %.to
	$(TAS) -d -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

%.coe: %.texe
	echo 'memory_initialization_radix = 16;' > $@
	echo 'memory_initialization_vector =' >> $@
	$(TAS) -d $< | $(TAS) -f text - | sed 's/^0x//' >> $@
	echo ';' >> $@

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
