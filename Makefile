CC = clang
CFLAGS  = -O3 -Wall

all: compress decompress

compress: compress.c
	$(CC) $(CFLAGS) -o compress compress.c

decompress: decompress.c
	$(CC) $(CFLAGS) -o decompress decompress.c

clean:
	$(RM) compress
	$(RM) decompress
