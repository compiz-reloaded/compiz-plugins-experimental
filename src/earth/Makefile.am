## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if EARTH_PLUGIN
libearth_la_LDFLAGS = $(PFLAGS)
libearth_la_LIBADD = @COMPIZ_LIBS@ @GLEW_LIBS@ @LIBCURL_LIBS@
nodist_libearth_la_SOURCES = earth_options.c earth_options.h
dist_libearth_la_SOURCES = earth.c earth.h
endif

BUILT_SOURCES = $(nodist_libearth_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

if EARTH_PLUGIN
module_LTLIBRARIES = libearth.la
endif

CLEANFILES = *_options.c *_options.h

earth_options.h: ../../metadata/earth.xml.in
		$(BCOP_BIN) --header $@ $<

earth_options.c: ../../metadata/earth.xml.in earth_options.h
		$(BCOP_BIN) --source $@ $<
