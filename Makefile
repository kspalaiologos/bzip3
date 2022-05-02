
CC=clang
CFLAGS=-O3 -march=native -mtune=native -flto

.PHONY: all clean

all: bzip3

%.o: %.c srt.h rle.h crc32.h cm.h mtf.h
	$(CC) $(CFLAGS) -c $< -o $@

bzip3: main.o libsais.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm bzip3 *.o
