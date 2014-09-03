TOP := $(abspath ..)
.SUFFIXES:
.SUFFIXES: .tas .to .texe
include $(TOP)/Makefile.common
include $(TOP)/Makefile.rules

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib
vpath %.tas.cpp $(TOP)/lib

TARGETS = forty.texe
clean_FILES += $(TARGETS) *.to

all: $(TARGETS)

forty.texe: main.to level0.to level1.to stringtable.to strtol.to memcpy.to

%.tas: %.tas.cpp forth_common.th
	cpp $(CPPFLAGS) $< -o $@

%.to: %.tas
	$(TAS) -o$@ $<

%.texe: %.to
	$(TLD) -o$@ $^

