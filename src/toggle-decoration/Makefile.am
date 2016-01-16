## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libtoggledeco_la_LDFLAGS = $(PFLAGS)
libtoggledeco_la_LIBADD = @COMPIZ_LIBS@
nodist_libtoggledeco_la_SOURCES = toggledeco_options.c toggledeco_options.h
dist_libtoggledeco_la_SOURCES = toggledeco.c

BUILT_SOURCES = $(nodist_libtoggledeco_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libtoggledeco.la

CLEANFILES = *_options.c *_options.h

toggledeco_options.h: ../../metadata/toggledeco.xml.in
		$(BCOP_BIN) --header $@ $<

toggledeco_options.c: ../../metadata/toggledeco.xml.in toggledeco_options.h
		$(BCOP_BIN) --source $@ $<
