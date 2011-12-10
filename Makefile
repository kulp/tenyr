CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -g
CFLAGS += -Wall -Wextra

all: tas tsim

tsim tas: ops.h

clean:
	$(RM) tas tsim *.o

