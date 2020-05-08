PEDANTIC = 1

ifeq ($(PEDANTIC),)
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
