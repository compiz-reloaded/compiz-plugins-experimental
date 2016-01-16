## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libfakeargb_la_LDFLAGS = $(PFLAGS)
libfakeargb_la_LIBADD = @COMPIZ_LIBS@
nodist_libfakeargb_la_SOURCES = fakeargb_options.c fakeargb_options.h
dist_libfakeargb_la_SOURCES = fakeargb.c

BUILT_SOURCES = $(nodist_libfakeargb_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libfakeargb.la

CLEANFILES = *_options.c *_options.h

fakeargb_options.h: ../../metadata/fakeargb.xml.in
		$(BCOP_BIN) --header $@ $<

fakeargb_options.c: ../../metadata/fakeargb.xml.in fakeargb_options.h
		$(BCOP_BIN) --source $@ $<
