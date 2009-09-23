CC = gcc
CFLAGS = -Wall -ansi -std=c99 -pedantic -g

vcp : vcp.o fileList.o
	$(CC) -o $@ $^

vcp.o : vcp.c
	$(CC) $(CFLAGS) -o $@ -c $<

fileList.o : fileList.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean :
	rm vcp *.o
