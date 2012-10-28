.PHONY: all tcc.js tas.js tas.bc
all: tcc.js tas.js

vpath %.bc ../../
ifeq ($(DEBUG),)
%.js: EMCCFLAGS += -O1
else
%.js: EMCCFLAGS += -O0
endif
tas.js: EMCCFLAGS += --pre-js build/scope_pre.js
tas.js: EMCCFLAGS += --post-js build/scope_post.js
tas.js: EMCCFLAGS += --js-transform "perl -i -p build/fsify.pl"
# TODO stop requiring -B
tas.bc:
	$(MAKE) -B -C ../.. $(basename $(notdir $@)).bc
tas.js: tas.bc
	emcc $(EMCCFLAGS) -o $@ $<

tcc.js:
	$(MAKE) -C build $(notdir $@)

