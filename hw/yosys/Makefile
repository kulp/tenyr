.DELETE_ON_ERROR:

YOSYS = yosys

vpath %.v ../verilog
vpath %.v ../../3rdparty/wb_intercon/rtl/verilog

YOSYS_FLAGS += -DVGA=1

VERILOG_SOURCES += ulx3s_top.v

VERILOG_SOURCES += clocks.v
VERILOG_SOURCES += gpio.v
VERILOG_SOURCES += seg7.v hex2segments.v
VERILOG_SOURCES += tmds.v tmds_pll.v
VERILOG_SOURCES += top.v tenyr.v ram.v
VERILOG_SOURCES += vgawrap.v vga_text.v
VERILOG_SOURCES += wb_mux.v
VERILOG_DEPS += default.memh

all: bit

bit: synthed.bit

synth: synthed.json

tmds_pll.v:
	ecppll --module tmds_pll --clkin 25 --clkout0 250 --clkout1 25 --file $@

synthed.json: YOSYS_FLAGS += -p "verilog_defaults -add -I../verilog"
synthed.json: YOSYS_FLAGS += $(foreach f,$(filter %.v,$^),-p "read_verilog $f")

synthed.json: $(VERILOG_SOURCES) $(VERILOG_DEPS)
	$(YOSYS) $(YOSYS_FLAGS) -p "hierarchy -top Top" -p "synth_ecp5 -json $@"

NEXTPNR = nextpnr-ecp5
NEXTPNR_FLAGS = --12k --lpf ulx3s_v20.lpf --package CABGA381

%.config: %.json ulx3s_v20.lpf
	$(NEXTPNR) $(NEXTPNR_FLAGS) --json $< --textcfg $@

%.bit: %.config
	ecppack --input $< --bit $@

TAS = tas

%.memh: %.tas
	$(TAS) -o $@ -fmemh $<
