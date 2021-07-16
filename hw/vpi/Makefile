makefile_path := $(abspath $(firstword $(MAKEFILE_LIST)))
TOP := $(CURDIR)/../..
include $(TOP)/mk/common.mk
include $(TOP)/mk/rules.mk

INCLUDES += $(TOP)/src
INCLUDES += $(INCLUDE_OS)

vpath %.c $(TOP)/src
# Explicitly add the directory containing the current Makefile to the VPATH so
# that this Makefile can be invoked from within BUILDDIR.
vpath %.c $(dir $(makefile_path))

all: vpi

.PHONY: vpi
vpi: vpidevices.vpi
vpidevices.vpi: callbacks,dy.o vpiserial,dy.o load,dy.o sim,dy.o asm,dy.o obj,dy.o common,dy.o param,dy.o stream,dy.o

