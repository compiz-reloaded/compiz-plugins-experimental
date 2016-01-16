## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libstar_la_LDFLAGS = $(PFLAGS)
libstar_la_LIBADD = @COMPIZ_LIBS@
nodist_libstar_la_SOURCES = star_options.c star_options.h
dist_libstar_la_SOURCES = star.c

BUILT_SOURCES = $(nodist_libstar_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libstar.la

CLEANFILES = *_options.c *_options.h

star_options.h: ../../metadata/star.xml.in
		$(BCOP_BIN) --header $@ $<

star_options.c: ../../metadata/star.xml.in star_options.h
		$(BCOP_BIN) --source $@ $<
