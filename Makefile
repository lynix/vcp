CC = gcc
CFLAGS = -Wall -ansi -std=c99 -pedantic -g
DESTDIR = /usr/local/bin

vcp : vcp.o lists.o file.o
	$(CC) -o $@ $^

vcp.o : vcp.c
	$(CC) $(CFLAGS) -o $@ -c $<

lists.o : lists.c
	$(CC) $(CFLAGS) -o $@ -c $<

file.o : file.c
	$(CC) $(CFLAGS) -o $@ -c $<

install: vcp
	install -m 755 vcp $(DESTDIR)

clean :
	rm -f vcp *.o

.PHONY: clean install
