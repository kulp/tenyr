PEDANTIC ?= 1

# Padding warnings are not really relevant to this project.
CFLAGS += -Wno-padded

# Do not warn about the unused version for endian reading.
obj.o obj,dy.o: CFLAGS += -Wno-unused-function

# Some casting away of qualifiers is currently deemed unavoidable, at least
# without running into different warnings.
tas.o tld.o param.o param,dy.o: CFLAGS += -W$(PEDANTRY_EXCEPTION)cast-qual
# Calls to variadic printf functions almost always need non-literal format
# strings.
common.o common,dy.o parser.o stream.o stream,dy.o: CFLAGS += -Wno-format-nonliteral

ifneq ($(PEDANTIC),1)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors -W$(PEDANTRY_EXCEPTION)unknown-warning-option

# Unreachable `return` statements after `fatal` are not an error for us.
PEDANTIC_FLAGS += -Wno-unreachable-code-return

# Our use of __DATE__ and __TIME__ is fine for us.
PEDANTIC_FLAGS += -Wno-date-time

# Required feature-macros trip these warnings spuriously.
PEDANTIC_FLAGS += -Wno-reserved-id-macro
PEDANTIC_FLAGS += -W$(PEDANTRY_EXCEPTION)cpp

# Hide needless -Wpoison-system-directories implied by -Weverything.
PEDANTIC_FLAGS += -Wno-poison-system-directories

# The following errors are meant to be absent, and therefore their presence is
# fatal except where elsewhere overridden on a per-object basis.
PEDANTIC_FLAGS += -Werror=switch-enum
PEDANTIC_FLAGS += -Werror=cast-qual
PEDANTIC_FLAGS += -Werror=vla
PEDANTIC_FLAGS += -Werror=strict-prototypes
PEDANTIC_FLAGS += -Werror=missing-prototypes
PEDANTIC_FLAGS += -Werror=unused-macros
PEDANTIC_FLAGS += -Werror=sign-conversion

endif
