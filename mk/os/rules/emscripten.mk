# Cannot use closure compiler on shared libraries
$(LIB_TARGETS:$(DYLIB_SUFFIX)=.js): CLOSURE_FLAGS =# empty
$(LIB_TARGETS:$(DYLIB_SUFFIX)=.js): LDFLAGS       = -s SIDE_MODULE=1
$(LIB_TARGETS:$(DYLIB_SUFFIX)=.js): EMCCFLAGS_LD  = -s SIDE_MODULE=1

tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

tsim.js: $(RSRC_FILES)
tsim.js: EMCCFLAGS_LD += $(RSRC_FLAGS)
tsim.js: EMCCFLAGS_LD += -s MAIN_MODULE=1

vpath %.c $(PP_BUILD)
vpath %.h $(PP_BUILD)
tcc.o: libtcc.c tccpp.c tccgen.c tcc.h libtcc.h tcctok.h
tcc.bc: CFLAGS := $(CC_DEBUG) -Wall
tcc.bc: CFLAGS += -Wno-pointer-sign -Wno-sign-compare -fno-strict-aliasing -Wno-shift-negative-value
tcc.bc: CFLAGS += $(CC_OPT)

%.js: %.bc
	@$(MAKESTEP) "[ EM-LD ] $@"
	$(EMCC) $(EMCCFLAGS_LD) $< $(LDLIBS) -o $@

# Disable closure compiler for now
tcc.js tas.js tsim.js tld.js: CLOSURE_FLAGS :=# empty

# Disable closing of streams so that the same code can run again
tas.bc tsim.bc tld.bc: CPPFLAGS += '-Dfclose=fflush'
