## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libstatic_la_LDFLAGS = $(PFLAGS)
libstatic_la_LIBADD = @COMPIZ_LIBS@
nodist_libstatic_la_SOURCES = static_options.c static_options.h
dist_libstatic_la_SOURCES = static.c

BUILT_SOURCES = $(nodist_libstatic_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libstatic.la

CLEANFILES = *_options.c *_options.h

static_options.h: ../../metadata/static.xml.in
		$(BCOP_BIN) --header $@ $<

static_options.c: ../../metadata/static.xml.in static_options.h
		$(BCOP_BIN) --source $@ $<
