
CC=clang
CFLAGS=-O3 -march=native -mtune=native -flto -Iinclude

.PHONY: all clean

OBJECTS=obj/main.o obj/libsais.o obj/crc32.o obj/mtf.o

all: bzip3

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bzip3: $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f bzip3 obj/*.o
