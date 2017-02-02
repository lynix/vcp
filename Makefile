CC	?= gcc
CFLAGS	= -std=c99 -Wall -pedantic -O2 -D_FILE_OFFSET_BITS=64 -pthread
LDFLAGS = -pthread
DESTDIR	= /usr/local

BIN	= bin/vcp
SRCS	= $(wildcard src/*.c)
OBJS	= $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

ifdef DEBUG
	CFLAGS += -g
endif

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

obj/%.o: src/%.c src/*.h
	$(CC) $(CFLAGS) -c -o $@ $<


install: $(BIN)
	mkdir -p $(DESTDIR)/bin
	install -m 755 $(BIN) $(DESTDIR)/bin/vcp

clean:
	rm -f $(BIN) $(OBJS)

.PHONY: all install clean

# vim: ts=8
