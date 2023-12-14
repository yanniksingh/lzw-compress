CC = clang
CFLAGS = -O3 -Wall

all: compress decompress

compress: compress.c common.h
	$(CC) $(CFLAGS) -o compress compress.c

decompress: decompress.c common.h
	$(CC) $(CFLAGS) -o decompress decompress.c

clean:
	$(RM) compress
	$(RM) decompress
