PEDANTIC = 1

ifeq ($(PEDANTIC),)
PEDANTIC_FLAGS ?= -pedantic
else
PEDANTIC_FLAGS ?= -Werror -pedantic-errors -Wno-error=unknown-warning-option

# Unreachable `return` statements after `fatal` are not an error for us.
PEDANTIC_FLAGS += -Wno-unreachable-code-return

# Our use of __DATE__ and __TIME__ is fine for us.
PEDANTIC_FLAGS += -Wno-date-time

# Required feature-macros trip this warning spuriously.
PEDANTIC_FLAGS += -Wno-reserved-id-macro

PEDANTIC_FLAGS += -Werror=covered-switch-default
PEDANTIC_FLAGS += -Werror=missing-variable-declarations
PEDANTIC_FLAGS += -Werror=switch-enum
PEDANTIC_FLAGS += -Werror=comma
PEDANTIC_FLAGS += -Werror=cast-qual
PEDANTIC_FLAGS += -Werror=vla

endif
