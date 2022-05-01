
CC=clang
CFLAGS=-O3 -march=native -mtune=native -flto -g3

.PHONY: all clean

all: bzip3

%.o: %.c mtf.h rle.h crc32.h cm.h
	$(CC) $(CFLAGS) -c $< -o $@

bzip3: main.o libsais.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm bzip3 *.o
