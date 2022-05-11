ACLOCAL_AMFLAGS = -I build-aux

AM_CFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS = -avoid-version

EXTRA_DIST = LICENSE PORTING.md README.md

include_HEADERS = include/cm.h \
				  include/common.h \
				  include/crc32.h \
				  include/libbz3.h \
				  include/libsais.h \
				  include/lzp.h \
				  include/rle.h

lib_LTLIBRARIES = libbzip3.la
libbzip3_la_SOURCES = src/cm.c \
					  src/crc32.c \
					  src/libbz3.c \
					  src/libsais.c \
					  src/lzp.c \
					  src/rle.c

bin_PROGRAMS = bzip3
bzip3_CFLAGS = $(AM_CFLAGS)
bzip3_LDDADD = libbzip3.la
bzip3_SOURCES = src/main.c $(libbzip3_la_SOURCES)
