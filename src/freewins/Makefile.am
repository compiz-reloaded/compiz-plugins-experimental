## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if FREEWINS_PLUGIN
libfreewins_la_LDFLAGS = $(PFLAGS)
libfreewins_la_LIBADD = @COMPIZ_LIBS@ -lGLU -lcairo
nodist_libfreewins_la_SOURCES = freewins_options.c freewins_options.h
dist_libfreewins_la_SOURCES = freewins.c
endif

BUILT_SOURCES = $(nodist_libfreewins_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if FREEWINS_PLUGIN
module_LTLIBRARIES = libfreewins.la
endif

CLEANFILES = *_options.c *_options.h

freewins_options.h: ../../metadata/freewins.xml.in
		$(BCOP_BIN) --header $@ $<

freewins_options.c: ../../metadata/freewins.xml.in freewins_options.h
		$(BCOP_BIN) --source $@ $<
