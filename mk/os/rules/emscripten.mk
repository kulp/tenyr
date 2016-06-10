tcc.js: EMCCFLAGS_LD += $(TH_FLAGS)

tsim.js: $(RSRC_FILES)
tsim.js: EMCCFLAGS_LD += $(RSRC_FLAGS)

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
