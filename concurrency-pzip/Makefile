CC=gcc
CFLAGS=-Wall -g -pthread

all: pzip

pzip: pzip.c
	$(CC) $(CFLAGS) -o pzip pzip.c

clean:
	rm -f pzip *.zip
