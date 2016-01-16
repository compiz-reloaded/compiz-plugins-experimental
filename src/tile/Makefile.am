## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libtile_la_LDFLAGS = $(PFLAGS)
libtile_la_LIBADD = @COMPIZ_LIBS@
nodist_libtile_la_SOURCES = tile_options.c tile_options.h
dist_libtile_la_SOURCES = tile.c

BUILT_SOURCES = $(nodist_libtile_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libtile.la

CLEANFILES = *_options.c *_options.h

tile_options.h: ../../metadata/tile.xml.in
		$(BCOP_BIN) --header $@ $<

tile_options.c: ../../metadata/tile.xml.in tile_options.h
		$(BCOP_BIN) --source $@ $<
