CC	= gcc
CFLAGS	= -Wall -ansi -std=c99 -pedantic -O2 -D_FILE_OFFSET_BITS=64 -pthread -g
LDFLAGS = -pthread
DESTDIR	= /usr/local

BIN	= bin/vcp
SRCS	= $(wildcard src/*.c)
OBJS	= $(addprefix obj/,$(notdir $(SRCS:.c=.o)))

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

.PHONY: clean install

# vim: ts=8
