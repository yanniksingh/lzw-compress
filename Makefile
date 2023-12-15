CC = gcc
CFLAGS = -O3 -Wall -std=c99 -pedantic-errors

all: compress uncompress

compress: compress.c common.h
	$(CC) $(CFLAGS) -o compress compress.c

uncompress: uncompress.c common.h
	$(CC) $(CFLAGS) -o uncompress uncompress.c

clean:
	$(RM) compress
	$(RM) uncompress
