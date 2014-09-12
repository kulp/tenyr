TOP := $(abspath ..)
.SUFFIXES:
.SUFFIXES: .tas .to .texe
include $(TOP)/mk/Makefile.common
include $(TOP)/mk/Makefile.rules

CPPFLAGS += -I$(TOP)/lib

vpath %.tas $(TOP)/lib
vpath %.tas.cpp $(TOP)/lib

CPP_FILES := $(wildcard *.tas.cpp)

TARGETS = forty.texe
clean_FILES += $(TARGETS) *.to
clean_FILES += $(CPP_FILES:%.tas.cpp=%.tas)

all: $(TARGETS)

forty.texe: main.to level0.to level1.to stringtable.to strtol.to memcpy.to

$(patsubst %.tas.cpp,%.tas,$(CPP_FILES)): forth_common.th

