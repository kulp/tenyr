CC = gcc
CFLAGS += -std=gnu99
CFLAGS += -g
CFLAGS += -Wall -Wextra

all: tas

tas: ops.h

clean:
	$(RM) tas *.o

