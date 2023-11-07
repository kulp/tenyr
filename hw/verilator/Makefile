include Vtop.mk

vpath %.v $(CURDIR)/../verilog
vpath %.v $(CURDIR)/../../3rdparty/wb_intercon/rtl/verilog

VFILES += top.v
VFILES += sim/simclocks.v
VFILES += tenyr.v
VFILES += ram.v
VFILES += seg7.v
VFILES += hex2segments.v
VFILES += gpio.v
VFILES += wb_mux.v

VFLAGS += -Wall
VFLAGS += -Wno-fatal
VFLAGS += -Wno-style

VFLAGS += -I$(CURDIR)/../verilog
VFLAGS += --cc --trace
VFLAGS += -DSIMCLK=tenyr_mainclock
VFLAGS += --Mdir $(CURDIR)
VFLAGS += --exe
# TODO drop --public-flat-rw
VFLAGS += --vpi --public-flat-rw

ifeq ($(DEBUG),1)
VFLAGS += --debug
CPPFLAGS += -DVL_DEBUG
endif

Vtop.mk: $(VFILES) sim_main.cpp
	verilator $(VFLAGS) $^
