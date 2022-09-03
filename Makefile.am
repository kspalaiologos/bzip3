ACLOCAL_AMFLAGS = -I build-aux

AM_CFLAGS = -I$(top_srcdir)/include
AM_LDFLAGS = -avoid-version

EXTRA_DIST = LICENSE PORTING.md README.md build-aux/git-version-gen

pkgconfig_DATA = bzip3.pc

include_HEADERS = include/libbz3.h
noinst_HEADERS = include/common.h \
						 include/libsais.h \
						 include/getopt-shim.h

lib_LTLIBRARIES = libbzip3.la
libbzip3_la_SOURCES = src/libbz3.c \
					  src/libsais.c

bin_PROGRAMS = bzip3
bzip3_CFLAGS = $(AM_CFLAGS)
bzip3_LDDADD = libbzip3.la
bzip3_SOURCES = src/main.c $(libbzip3_la_SOURCES)

dist_man_MANS = bzip3.1 bz3cat.1 bz3more.1 bz3less.1 bz3grep.1

dist_bin_SCRIPTS = bz3cat bz3more bz3less bz3grep unbzip3

CLEANFILES = $(bin_PROGRAMS)

# End standard generic autotools stuff

# Begin special handling for autoconf VERSION being updated on commit

BUILT_SOURCES = .version
CLEANFILES += $(BUILT_SOURCES) .version-prev

src/bzip3-main.$(OBJEXT): .version

_BRANCH_REF != $(AWK) '{print ".git/" $$2}' .git/HEAD 2>/dev/null ||:

.version: $(_BRANCH_REF)
	@if [ -e "$(srcdir)/.tarball-version" ]; then \
		printf "$(VERSION)" > $@; \
	else \
		touch "$@-prev"; \
		if [ -e "$@" ]; then \
			cp "$@" "$@-prev"; \
		fi; \
		./build-aux/git-version-gen "$(srcdir)/.tarball-version" > $@; \
		cmp -s "$@" "$@-prev" || autoreconf configure.ac --force; \
	fi

dist-hook:
	printf "$(VERSION)" > "$(distdir)/.tarball-version"

# Begin developer convenience targets

.PHONY: format
format: $(bzip3_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	clang-format -i $^

.PHONY: cloc
cloc: $(bzip3_SOURCES) $(include_HEADERS) $(noinst_HEADERS)
	cloc $^

CLEANFILES += LICENSE2
.PHONY: roundtrip

BZIP3 := bzip3$(EXEEXT)

roundtrip: $(BZIP3)
	rm -f LICENSE2
	./$(BZIP3) -feb 6 LICENSE
	./$(BZIP3) -d LICENSE.bz3 LICENSE2
	cmp LICENSE LICENSE2
