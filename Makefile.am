ACLOCAL_AMFLAGS = -I build-aux

AM_CFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS = -avoid-version

EXTRA_DIST = LICENSE PORTING.md README.md build-aux/git-version-gen

include_HEADERS = include/libbz3.h
noinst_HEADERS = include/cm.h \
						 include/common.h \
						 include/crc32.h \
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

CLEANFILES = $(bin_PROGRAMS)

# End standard generic autotools stuff

# Begin special handling for autoconf VERSION being updated on commit

BUILT_SOURCES = .version
CLEANFILES += $(BUILT_SOURCES) .version-prev .version-next

src/bzip3-main.$(OBJEXT): .version

_BRANCH_REF != $(AWK) '{print ".git/" $$2}' .git/HEAD 2>/dev/null

.version: $(_BRANCH_REF)
	if [ -e "$@" ]; then \
		mv "$@" "$@-prev"; \
	else \
		[ -e "$<" ] && touch "$@-prev" || cp "$(srcdir)/.tarball-version" "$@-prev"; \
	fi
	if [ -e "$<" ]; then \
		./build-aux/git-version-gen "$(srcdir)/.tarball-version"; \
		cmp -s "$@" "$@-prev" || autoreconf configure.ac --force; \
	else \
		printf "$(VERSION)"; \
	fi > $@

dist-hook:
	printf "$(VERSION)" > "$(distdir)/.tarball-version"

# Begin developer convenience targets

.PHONY: format
format: $(bzip3_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	clang-format -i $^

.PHONY: cloc
cloc: $(bzip3_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	cloc $^

.PHONY: roundtrip
roundtrip: $(bin_PROGRAMS)
	time ./$< -e -b 6 etc/shakespeare.txt
	time ./$< -d etc/shakespeare.txt.bz3
