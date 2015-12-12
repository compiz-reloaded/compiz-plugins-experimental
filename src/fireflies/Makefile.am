## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libfireflies_la_LDFLAGS = $(PFLAGS)
libfireflies_la_LIBADD = @COMPIZ_LIBS@
nodist_libfireflies_la_SOURCES = fireflies_options.c fireflies_options.h
dist_libfireflies_la_SOURCES = fireflies.c

BUILT_SOURCES = $(nodist_libfireflies_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libfireflies.la

CLEANFILES = *_options.c *_options.h

fireflies_options.h: ../../metadata/fireflies.xml.in
		$(BCOP_BIN) --header $@ $<

fireflies_options.c: ../../metadata/fireflies.xml.in fireflies_options.h
		$(BCOP_BIN) --source $@ $<
