ifneq ($(SDL),0)
sdl2_pkg_config ?= $(shell $(SDL2_PKGCONFIG) $1 $2)
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
# Mark SDL header includes as system includes to avoid subjecting them to full
# pedantry
libtenyrsdl%$(DYLIB_SUFFIX) $(PDEVICES_SDL:%=devices/%.d): \
    CPPFLAGS += $(patsubst -I%,-isystem %,$(call sdl2_pkg_config,--cflags))
libtenyrsdl%$(DYLIB_SUFFIX): LDLIBS += $(call sdl2_pkg_config,--libs) -lSDL2_image
libtenyrsdlvga$(DYLIB_SUFFIX): | $(TOP)/rsrc/font10x15/invert.font10x15.png
$(TOP)/rsrc/font10x15/%:
	$(MAKE) -C $(@D) $(@F)

# Do not halt the build for platforms on which the feature-flag _BSD_SOURCE is
# unused.
sdl%.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)unused-macros -W'$(PEDANTRY_EXCEPTION)\#warnings'
endif

