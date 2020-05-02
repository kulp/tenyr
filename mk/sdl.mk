ifneq ($(SDL),0)
sdl2_pkg_config ?= $(shell $(SDL2_PKGCONFIG) $1 $2)
SDL_VERSION := $(call sdl2_pkg_config,--modversion,sdl2)
ifneq ($(SDL_VERSION),)
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
libtenyrsdl%$(DYLIB_SUFFIX) $(PDEVICES_SDL:%=devices/%.d): \
    CPPFLAGS += $(call sdl2_pkg_config,--cflags,sdl2 SDL2_image)
libtenyrsdl%$(DYLIB_SUFFIX): LDLIBS += $(call sdl2_pkg_config,--libs,sdl2 SDL2_image)
libtenyrsdlvga$(DYLIB_SUFFIX): | $(TOP)rsrc/font10x15/invert.font10x15.png
$(TOP)rsrc/font10x15/%:
	$(MAKE) -C $(@D) $(@F)

# Do not halt the build for platforms on which the feature-flag _BSD_SOURCE is
# unused.
sdl%.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)unused-macros -W'$(PEDANTRY_EXCEPTION)#warnings'
endif
endif

