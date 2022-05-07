
CC=clang
CFLAGS=-O2 -march=native -mtune=native -Iinclude -g3 -fPIC
PREFIX?=/usr/local

.PHONY: all clean format install cloc

LIBBZ3_OBJECTS=obj/libsais.o obj/crc32.o obj/rle.o obj/cm.o \
               obj/libbz3.o obj/lzp.o

all: bzip3 bzip3.so

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

bzip3.so: $(LIBBZ3_OBJECTS)
	$(CC) -shared $(CFLAGS) -o $@ $^ -lpthread 

bzip3: obj/main.o $(LIBBZ3_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread 

clean:
	rm -f bzip3 obj/*.o

format:
	clang-format -i src/*.c include/*.h

install:
	install -c -v -m 755 bzip3 $(PREFIX)/bin
	install -c -v -m 755 bzip3.so $(PREFIX)/lib
	install -c -v -m 755 include/libbz3.h $(PREFIX)/include

cloc:
	cloc src/*.c include/*.h
