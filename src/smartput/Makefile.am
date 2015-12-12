## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libsmartput_la_LDFLAGS = $(PFLAGS)
libsmartput_la_LIBADD = @COMPIZ_LIBS@
nodist_libsmartput_la_SOURCES = smartput_options.c smartput_options.h
dist_libsmartput_la_SOURCES = smartput.c

BUILT_SOURCES = $(nodist_libsmartput_la_SOURCES)

AM_CFLAGS =                                \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libsmartput.la

CLEANFILES = *_options.c *_options.h

smartput_options.h: ../../metadata/smartput.xml.in
		$(BCOP_BIN) --header $@ $<

smartput_options.c: ../../metadata/smartput.xml.in smartput_options.h
		$(BCOP_BIN) --source $@ $<
