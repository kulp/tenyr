TAS = ../../tas
SHELL = /bin/bash
SETUP = source ~/ise.sh
TARGET = Tenyr

all: $(TARGET).bit

burn: $(TARGET).bit
	djtgcfg -d Nexys3 -i 0 -f $< prog

vpath %.tas ../verilog

%.to: %.tas
	$(TAS) $< > $@

%.tasd: %.to
	$(TAS) -d $< > $@

%.coe: %.tasd
	echo 'memory_initialization_radix = 16;' > $@
	echo 'memory_initialization_vector =' >> $@
	$(TAS) -f text $< | sed 's/^0x//' >> $@
	echo ';' >> $@

vpath gen_%.tcl ipcore_dir
vpath %.xco ipcore_dir
vpath %.ngc ipcore_dir

%.ngc: gen_%.tcl
	$(SETUP) && cd $(dir $<) && xtclsh $(notdir $<)

GenedBlockMem.ngc: GenedBlockMem.xco small.coe

vpath %.v ../verilog

Tenyr_SRCDEPS += \
	hex2ascii.v \
	seg7.v \
	seglookup.v \
	serial.v \
	tenyr.v \
	top.v \
	Nexys3_Master.ucf \
	Tenyr.ucf \
	GenedBlockMem.xco \
	tenyr_clk_S.xco \
	xilinx_top.v

# TODO pull tenyr.tcl components into Makefile to allow incremental builds
%.bit: $($*_SRCDEPS) GenedBlockMem.ngc
	$(SETUP) && xtclsh $*.tcl run_process

