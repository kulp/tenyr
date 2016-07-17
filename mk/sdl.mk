ifneq ($(SDL),0)
sdl2_pkg_config ?= $(shell $(SDL2_PKGCONFIG) $1 $2)
SDL_VERSION := $(call sdl2_pkg_config,--modversion,sdl2)
ifneq ($(SDL_VERSION),)
# Use := to ensure the expensive underyling call is not repeated
NO_C11_WARN_OPTS := $(call cc_flag_supp,-Wno-c11-extensions)
PDEVICES_SDL += sdlled sdlvga
PDEVICES += $(PDEVICES_SDL)
libtenyrsdl%$(DYLIB_SUFFIX) $(PDEVICES_SDL:%=devices/%.d): \
    CPPFLAGS += $(call sdl2_pkg_config,--cflags,sdl2 SDL2_image) $(NO_C11_WARN_OPTS)
libtenyrsdl%$(DYLIB_SUFFIX): LDLIBS += $(call sdl2_pkg_config,--libs,sdl2 SDL2_image)
$(PDEVICES_SDL:%=%,dy.o): PEDANTIC_FLAGS :=
endif
endif

