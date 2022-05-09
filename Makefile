
CC?=clang
CFLAGS?=-O2 -march=native -mtune=native -Iinclude -flto -g3 -fPIC
PREFIX?=/usr/local

.PHONY: all clean format install cloc check

LIBBZ3_OBJECTS=obj/libsais.o obj/crc32.o obj/rle.o obj/cm.o \
               obj/libbz3.o obj/lzp.o

all: bzip3 libbzip3.so

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

libbzip3.so: $(LIBBZ3_OBJECTS)
	$(CC) -shared $(CFLAGS) -o $@ $^ -lpthread 

bzip3: obj/main.o $(LIBBZ3_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lpthread 

clean:
	rm -f bzip3 libbzip3.so obj/*.o

format:
	clang-format -i src/*.c include/*.h

install:
	install -c -v -m 755 bzip3 $(PREFIX)/bin
	install -c -v -m 755 libbzip3.so $(PREFIX)/lib
	install -c -v -m 755 include/libbz3.h $(PREFIX)/include

cloc:
	cloc src/*.c include/*.h

check: bzip3
	time ./bzip3 -e -b 6 etc/shakespeare.txt
	time ./bzip3 -d etc/shakespeare.txt.bz3
