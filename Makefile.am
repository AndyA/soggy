DIST_SUBDIRS = m4

SUBDIRS = .

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = soggy

soggy_SOURCES = soggy.c utils.c utils.h

soggy_CFLAGS = $(LIBOGG_CFLAGS)
soggy_LDFLAGS = $(LIBOGG_LDFLAGS)

test: all
	cd t && $(MAKE) test

