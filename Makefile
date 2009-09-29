CC = gcc
CFLAGS = -Wall -ansi -std=c99 -pedantic -g

vcp : vcp.o lists.o file.o
	$(CC) -o $@ $^

vcp.o : vcp.c
	$(CC) $(CFLAGS) -o $@ -c $<

lists.o : lists.c
	$(CC) $(CFLAGS) -o $@ -c $<

file.o : file.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	rm vcp *.o
