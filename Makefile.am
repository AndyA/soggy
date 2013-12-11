DIST_SUBDIRS = m4

SUBDIRS = .

ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = soggy

soggy_SOURCES = soggy.c segname.c segname.h utils.c utils.h

soggy_CFLAGS = $(libogg_CFLAGS) $(libvorbis_CFLAGS)
soggy_LDFLAGS = $(libogg_LIBS) $(libvorbis_LIBS)

test: all
	cd t && $(MAKE) test

